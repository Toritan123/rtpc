#include "memory_heap.h"
#ifdef PLATFORM_PC
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#ifndef PLATFORM_PC
asm(".include \"include/gba.inc\"");//Temporary
#endif

#ifdef PLATFORM_PC
// ─── PC malloc-based heap ─────────────────────────────────────────────────────
// On GBA the heap lives inside EWRAM (256 KB), which is far too small once
// x86-64 pointer sizes (8 bytes vs 4 bytes) double the size of every struct.
// On PC we back every allocation with a plain malloc so there is no size limit.
//
// We keep a flat table of { ptr, id } so we can implement dealloc_with_id.
// 4096 simultaneous live allocations is generous for a GBA game.

#define PC_HEAP_MAX_ALLOCS 4096

struct PcHeapEntry {
    void *ptr;
    u16   id;
};

static struct PcHeapEntry pc_heap[PC_HEAP_MAX_ALLOCS];
static int pc_heap_used = 0;

void mem_heap_init(u32 *heapStart, u32 heapSize) {
    (void)heapStart; (void)heapSize;
    // free any left-over allocations from a previous heap init
    for (int i = 0; i < PC_HEAP_MAX_ALLOCS; i++) {
        if (pc_heap[i].ptr) {
            free(pc_heap[i].ptr);
            pc_heap[i].ptr = NULL;
        }
    }
    pc_heap_used = 0;
    memset(&D_03004ad0, 0, sizeof(D_03004ad0));
}

void *mem_heap_alloc(u32 size) {
    return mem_heap_alloc_id(0, size);
}

void *mem_heap_alloc_id(u16 id, u32 size) {
    if (size == 0) size = 4;

    // Find an empty slot
    int slot = -1;
    for (int i = 0; i < PC_HEAP_MAX_ALLOCS; i++) {
        if (!pc_heap[i].ptr) { slot = i; break; }
    }
    if (slot < 0) {
        fprintf(stderr, "[HEAP-PC] alloc table full (max %d live allocs)\n",
                PC_HEAP_MAX_ALLOCS);
        D_03004ad0.unk0 = 1;
        return NULL;
    }

    void *p = calloc(1, size);
    if (!p) {
        fprintf(stderr, "[HEAP-PC] malloc(%u) failed\n", size);
        D_03004ad0.unk0 = 1;
        return NULL;
    }

    pc_heap[slot].ptr = p;
    pc_heap[slot].id  = id;
    if (slot >= pc_heap_used) pc_heap_used = slot + 1;
    return p;
}

// mem_heap_dealloc_block is not needed on PC (GBA-internal helper).
void mem_heap_dealloc_block(u32 block, s32 prevBlock) {
    (void)block; (void)prevBlock;
}

void mem_heap_dealloc(void *data) {
    if (!data) return;
    for (int i = 0; i < pc_heap_used; i++) {
        if (pc_heap[i].ptr == data) {
            free(pc_heap[i].ptr);
            pc_heap[i].ptr = NULL;
            pc_heap[i].id  = 0;
            return;
        }
    }
    // Not found in our table — could be a stack/static pointer; ignore.
}

void mem_heap_dealloc_with_id(u16 id) {
    if (id == 0) return;  // matches GBA behaviour
    for (int i = 0; i < pc_heap_used; i++) {
        if (pc_heap[i].ptr && pc_heap[i].id == id) {
            free(pc_heap[i].ptr);
            pc_heap[i].ptr = NULL;
            pc_heap[i].id  = 0;
        }
    }
}

void mem_heap_get_allocated_space(void) {
    u32 total = 0;
    for (int i = 0; i < pc_heap_used; i++) {
        if (pc_heap[i].ptr) total += 16; // rough estimate per slot
    }
    D_03004ad0.unkC = total;
}

#else  // GBA ──────────────────────────────────────────────────────────────────

// These functions handle allocation and deallocation from the main memory heap.
// The heap consists of a sequence of variable-sized blocks of data.
// Each block consists of a single word handle, immediately followed by the data the handle refers to.
// The handle stores the size of the block, if the block is currently allocated, and an ID.
// The size of the block can be used to get to the next handle, creating a singly linked list.

// The heap is initialised as a single unallocated block spanning the entire heap.
// Allocation works by first fit to find an unallocated block large enough for the requested data.
// If the block is larger than the required size, it will split the remainder of the block into a new smaller unallocated block.
// When a block is deallocated, the handle is simply marked as unallocated but the data is not cleared.
// Additionally, a deallocated block will be combined with the adjacent blocks if they are unallocated, to create one larger block.

extern void *mem_heap_alloc_block_rom;
extern void *mem_heap_alloc_block_rom_end;
static u32 mem_heap_alloc_block_code[20];

static s32 (*mem_heap_alloc_block)(u32 *memHeap, s32 memHeapSize, s32 length);

static u32 *sMemoryHeap;
static u32 sMemoryHeapLength;
#define MEM_HANDLE_ALLOCATED 0x8000

// Initialise the main memory heap.
void mem_heap_init(u32 *heapStart, u32 heapSize) {
    sMemoryHeap = heapStart;
    sMemoryHeapLength = heapSize / 4;

	// Make sure the memory heap does not extend outside of EWRAM.
    if (sMemoryHeapLength > 0xFFFF) {
        sMemoryHeapLength = 0xFFFF;
    }

	// Create the initial unallocated block's handle, spanning the length of the whole pool.
    sMemoryHeap[0] = sMemoryHeapLength << 16;

    D_03004ad0.unk0 = 0;
    D_03004ad0.unk4 = heapStart;
    D_03004ad0.unk8 = heapSize;
    D_03004ad0.unk10 = 0;
    D_03004ad0.unkC = 0;

	// The allocation function is handwritten assembly DMAd into IWRAM for performance.
	DmaCopy32(3, &mem_heap_alloc_block_rom, &mem_heap_alloc_block_code, ((uintptr_t)&mem_heap_alloc_block_rom_end - (uintptr_t)&mem_heap_alloc_block_rom));
    mem_heap_alloc_block = (void *)&mem_heap_alloc_block_code;
}


// Allocate a new block of memory from the memory heap with the default ID.
void *mem_heap_alloc(u32 size) {
	return mem_heap_alloc_id(0, size);
}


// Allocate from the memory heap with an ID.
void *mem_heap_alloc_id(u16 id, u32 size) {
    u32 blockLength = (size+3)/4 + 1; // Add 1 to the length so there is room for the block's handle as well.
	s32 oldBlockLength, blockEnd;

	// Find an unallocated block of memory using first fit.
    s32 newBlock = mem_heap_alloc_block(sMemoryHeap, sMemoryHeapLength, blockLength);

    if (newBlock < 0) {
		// There were no unallocated blocks large enough, abort the allocation.
        D_03004ad0.unk0 = 1;
        return NULL;
    }

    oldBlockLength = (u16)(sMemoryHeap[newBlock] >> 16);

	// Create the handle for the newly allocated block.
    sMemoryHeap[newBlock] = (blockLength << 16) | MEM_HANDLE_ALLOCATED | id;

    blockEnd = newBlock + blockLength;

	// If the requested block was smaller than the unallocated block it replaced,
	// we can turn the unused memory into a smaller unallocated block.
    if (blockLength < oldBlockLength) {
        sMemoryHeap[blockEnd] = (oldBlockLength - blockLength) << 16;
    }

    if ((blockEnd * 4) > D_03004ad0.unk10) {
        D_03004ad0.unk10 = (blockEnd * 4);
    }

	// Return the pointer to the allocated data, which is immediately after the new block's handle.
    return &(sMemoryHeap[newBlock]) + 1;
}


// Mark a block as deallocated and try to combine it with adjacent deallocated blocks, if possible.
void mem_heap_dealloc_block(u32 block, s32 prevBlock) {
    u32 nextBlock;

	// Clear the block's allocated flag and ID.
    sMemoryHeap[block] &= 0xFFFF0000;

	// If the preceding block is also unallocated, combine them into one larger block.
    if (prevBlock >= 0) {
        if ((sMemoryHeap[prevBlock] & MEM_HANDLE_ALLOCATED) == 0) {
            sMemoryHeap[prevBlock] += sMemoryHeap[block];
            block = prevBlock;
        }
    }

	// If the following block is also unallocated, combine them into one larger block.
    nextBlock = block + (sMemoryHeap[block] >> 16);
    if (nextBlock < sMemoryHeapLength) {
        if ((sMemoryHeap[nextBlock] & MEM_HANDLE_ALLOCATED) == 0) {
            sMemoryHeap[block] += sMemoryHeap[nextBlock];
        }
    }
}


// Deallocate the block belonging to a previously allocated section of data.
void mem_heap_dealloc(void *data) {
    s32 block, curBlock, prevBlock;

	// Make sure the data is word-aligned
    if ((uintptr_t)data & 0x3) {
        return;
    }

	// Get the location of the block's handle within the heap. Subtract 1 as the handle is immediately before the data.
    block = ((uintptr_t)data - (uintptr_t)sMemoryHeap) / 4 - 1;

    if ((block < 0) || (block >= sMemoryHeapLength)) {
		// Data is not part of the memory heap
        return;
    }

    prevBlock = -1;
    curBlock = 0;

    if (curBlock >= sMemoryHeapLength) {
        return;
    }

	// Iterate through the freelist to find the block and deallocate it.
	// This is required so that the previous block's handle can also be passed to the deallocation function,
	// as the freelist is only a singly linked list and so the previous block can't be obtained otherwise.
    while (curBlock <= block) {
        if (curBlock == block) {
            mem_heap_dealloc_block(block, prevBlock);
            break;
        }

        prevBlock = curBlock;
		curBlock += sMemoryHeap[curBlock] >> 16;

        if (curBlock >= sMemoryHeapLength) {
            break;
        }
    }
}


// Deallocate all blocks in the heap that have a specified ID.
void mem_heap_dealloc_with_id(u16 id) {
    s32 curBlock, prevBlock;
	u32 handleID;

	// Cannot deallocate blocks with the default ID.
    if (id == 0) {
        return;
    }

    prevBlock = -1;
	// The lower 16 bits of the handle, to match all allocated blocks with the given ID.
    handleID = id | MEM_HANDLE_ALLOCATED;
    curBlock = 0;

	// Iterate through the freelist and deallocate any block that matches the ID.
    while (curBlock < sMemoryHeapLength) {
        if ((u16)sMemoryHeap[curBlock] == handleID) {
            mem_heap_dealloc_block(curBlock, prevBlock);
        }

        prevBlock = curBlock;
        curBlock += sMemoryHeap[curBlock] >> 16;
    }
}


// Calculates the total amount of allocated data in the heap.
void mem_heap_get_allocated_space(void) {
    u32 allocatedWords = 0;
    u32 curBlock = 0;

	// Iterate through the freelist and store the total length of allocated data.
    while (curBlock < sMemoryHeapLength) {
        if (sMemoryHeap[curBlock] & MEM_HANDLE_ALLOCATED) {
            allocatedWords += sMemoryHeap[curBlock] >> 16;
        }
        curBlock += sMemoryHeap[curBlock] >> 16;
    }

    D_03004ad0.unkC = allocatedWords * 4;
}

#endif // PLATFORM_PC
