#include "affine_param.h"
#include "memory_heap.h"

#ifndef PLATFORM_PC
asm(".include \"include/gba.inc\"");//Temporary
#endif


/* ROTATION/SCALING PARAMETERS */


static s32 D_03000138[64]; // unknown type
static s32 D_03000238[64]; // unknown type
static s32 D_03000338[2]; // unknown type
static s32 D_03000340[8]; // unknown type
static s32 D_03000360[2]; // unknown type
static s32 D_03000368[32]; // unknown type


#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_080020ec.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_08002150.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_08002194.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_080021b8.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_0800222c.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_08002260.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_08002280.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_080022bc.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_080022d8.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_080022f4.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_08002310.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_0800232c.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_080024dc.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_08002500.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_08002520.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_0800253c.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_08002584.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_080025bc.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_080025d8.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08001360/asm_080025fc.s"
#endif
