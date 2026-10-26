#include "global.h"
#include "src/code_08007468.h"
#include "src/affine_param.h"
#include "src/code_08003b28.h"
#include "src/bitmap_font.h"
#include "src/memory_heap.h"
#include "src/lib_0804ca80.h"

// Can be better split

#ifdef PLATFORM_PC // declarations this file used without a prototype
#include "syscall.h"
#include "src/code_08001360.h"
#include "src/code_080068f8.h"
#endif

#ifndef PLATFORM_PC
asm(".include \"include/gba.inc\"");//Temporary
#endif


/* GRAPHICS UTIL */


// Get Sprite XY
void get_sprite_xy(s16 sprite, s16 *xReq, s16 *yReq) {
    *xReq = sprite_get_data(gSpriteHandler, sprite, 4);
    *yReq = sprite_get_data(gSpriteHandler, sprite, 5);
}


// Set Sprite Affine Param.
void assign_sprite_affine_param(s16 sprite, s8 affineIndex) {
    sprite_set_affine_params(gSpriteHandler, sprite, affineIndex, func_08002520(affineIndex));
}


// Set Affine Parameters
void set_affine_scale_rotation(s8 affineParam, s16 scale, s16 rotation) {
    func_0800232c(affineParam, scale, scale, -rotation, 1);
}


// Set Affine Parameters
void set_affine_stretch_rotation(s8 affineParam, s16 xScale, s16 yScale, s16 rotation) {
    func_0800232c(affineParam, xScale, yScale, -rotation, 1);
}


// Sprite Motion - Indefinite Linear (Start)
struct SpriteMover_Indefinite *start_sprite_motion_task_indefinite(struct SpriteMover_Indefinite_Inputs *inputs) {
    struct SpriteMover_Indefinite *task;

    if (inputs->sprite < 0) {
        return TASK_FAILED_TO_START;
    }

    task = mem_heap_alloc(sizeof(struct SpriteMover_Indefinite));
    task->sprite = inputs->sprite;
    task->posX = INT_TO_FIXED(inputs->startX);
    task->posY = INT_TO_FIXED(inputs->startY);
    task->velX = inputs->velX;
    task->velY = inputs->velY;

    sprite_set_x_y(gSpriteHandler, inputs->sprite, inputs->startX, inputs->startY);
    return task;
}


// Sprite Motion - Indefinite Linear (Update)
u32 update_sprite_motion_task_indefinite(struct SpriteMover_Indefinite *task) {
    task->posX += task->velX;
    task->posY += task->velY;

    sprite_set_x_y(gSpriteHandler, task->sprite, FIXED_TO_INT(task->posX), FIXED_TO_INT(task->posY));
    return FALSE;
}


// Sprite Motion - Decelerate to Point (Start)
struct SpriteMover_Decelerate *start_sprite_motion_task_decelerate(struct SpriteMover_Decelerate_Inputs *inputs) {
    struct SpriteMover_Decelerate *task;

    if (inputs->sprite < 0) {
        return TASK_FAILED_TO_START;
    }

    task = mem_heap_alloc(sizeof(struct SpriteMover_Decelerate));
    task->sprite = inputs->sprite;
    task->destX = inputs->destX;
    task->destY = inputs->destY;
    task->dx = INT_TO_FIXED(inputs->startX - inputs->destX);
    task->dy = INT_TO_FIXED(inputs->startY - inputs->destY);
    task->multiplier = inputs->multiplier;

    sprite_set_x_y(gSpriteHandler, inputs->sprite, inputs->startX, inputs->startY);
    return task;
}


// Sprite Motion - Decelerate to Point (Update)
u32 update_sprite_motion_task_decelerate(struct SpriteMover_Decelerate *task) {
    s32 dx = task->dx;
    s32 dy = task->dy;

    dx = ABS(dx);
    dy = ABS(dy);

    dx = FIXED_POINT_MUL(dx, task->multiplier);
    dy = FIXED_POINT_MUL(dy, task->multiplier);

    if (dx < INT_TO_FIXED(1.0)) dx = 0;
    if (dy < INT_TO_FIXED(1.0)) dy = 0;

    if (task->dx < 0) dx = -dx;
    if (task->dy < 0) dy = -dy;

    task->dx = dx;
    task->dy = dy;

    sprite_set_x_y(gSpriteHandler, task->sprite, FIXED_TO_INT(dx) + task->destX, FIXED_TO_INT(dy) + task->destY);

    if ((dx | dy) == 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}


// Sprite Motion - Accelerate to Point (Start)
struct SpriteMover_Accelerate *start_sprite_motion_task_accelerate(struct SpriteMover_Accelerate_Inputs *inputs) {
    struct SpriteMover_Accelerate *task;

    if (inputs->sprite < 0) {
        return TASK_FAILED_TO_START;
    }

    task = mem_heap_alloc(sizeof(struct SpriteMover_Accelerate));
    task->sprite = inputs->sprite;
    task->startX = inputs->startX;
    task->startY = inputs->startY;
    task->dx = inputs->destX - inputs->startX;
    task->dy = inputs->destY - inputs->startY;
    task->distanceTravelled = 0;
    task->totalDistance = INT_TO_FIXED(math_sqrt(task->dx * task->dx + task->dy * task->dy));
    task->velocity = inputs->velocity;
    task->acceleration = inputs->acceleration;

    sprite_set_x_y(gSpriteHandler, inputs->sprite, inputs->startX, inputs->startY);
    return task;
}


// Sprite Motion - Accelerate to Point (Update)
u32 update_sprite_motion_task_accelerate(struct SpriteMover_Accelerate *task) {
    s24_8 distTravelled, totalDist;
    u32 reachedEnd;
    s16 x, y;

    task->distanceTravelled += task->velocity;
    task->velocity += task->acceleration;

    if (task->velocity < 0 && task->acceleration < 0) {
        return TRUE;
    }

    distTravelled = task->distanceTravelled;
    totalDist = task->totalDistance;
    reachedEnd = FALSE;

    if (distTravelled >= totalDist) {
        distTravelled = totalDist;
        reachedEnd = TRUE;
    }

    if (reachedEnd) {
        x = task->startX + task->dx;
        y = task->startY + task->dy;
    } else {
        x = task->startX + lerp(0, task->dx, distTravelled, totalDist);
        y = task->startY + lerp(0, task->dy, distTravelled, totalDist);
    }

    sprite_set_x_y(gSpriteHandler, task->sprite, x, y);
    return reachedEnd;
}


// Sprite Motion - LERP to Point (Start)
struct SpriteMover_TimedLinear *start_sprite_motion_task_lerp(struct SpriteMover_TimedLinear_Inputs *inputs) {
    struct SpriteMover_TimedLinear *task;

    if (inputs->sprite < 0) {
        return TASK_FAILED_TO_START;
    }

    task = mem_heap_alloc(sizeof(struct SpriteMover_TimedLinear));
    task->sprite = inputs->sprite;
    task->startX = inputs->startX;
    task->startY = inputs->startY;
    task->dx = inputs->destX - inputs->startX;
    task->dy = inputs->destY - inputs->startY;
    task->totalFrames = inputs->totalFrames;
    task->framesPassed = 0;

    sprite_set_x_y(gSpriteHandler, inputs->sprite, inputs->startX, inputs->startY);
    return task;
}


// Sprite Motion - LERP to Point (Update)
u32 update_sprite_motion_task_lerp(struct SpriteMover_TimedLinear *task) {
    s32 totalFrames = task->totalFrames;
    u32 reachedEnd = (++task->framesPassed >= totalFrames);
    s32 framesPassed = task->framesPassed;
    s16 x = task->startX + lerp(0, task->dx, framesPassed, totalFrames);
    s16 y = task->startY + lerp(0, task->dy, framesPassed, totalFrames);

    sprite_set_x_y(gSpriteHandler, task->sprite, x, y);
    return reachedEnd;
}


// Sprite Motion - Sinusoidal Oscillation (Update)
u32 update_sprite_motion_task_sine_osc(struct SpriteMover_SineOsc *task) {
    u8 wavePos = lerp(task->waveStart, task->waveEnd, task->framesPassed, task->totalFrames);

    s32 offset = task->baseOffset + FIXED_TO_INT(task->amplitude * sins2(wavePos));
    
    u16 xPos = FIXED_TO_INT(offset * coss2(task->angle)) + task->baseXPos;
    u16 yPos = FIXED_TO_INT(offset * sins2(task->angle)) + task->baseYPos;

    sprite_set_x_y(gSpriteHandler, task->sprite, xPos, yPos);

    return (++task->framesPassed > task->totalFrames);
}


// Sprite Motion - Sinusoidal Oscillation (Start)
struct SpriteMover_SineOsc *start_sprite_motion_task_sine_osc(struct SpriteMover_SineOsc_Inputs *inputs) {
    struct SpriteMover_SineOsc *task;

    if (inputs->sprite < 0) {
        return TASK_FAILED_TO_START;
    }

    task = mem_heap_alloc(sizeof(struct SpriteMover_SineOsc));
    task->sprite = inputs->sprite;
    task->baseXPos = inputs->baseX;
    task->baseYPos = inputs->baseY;
    task->baseOffset = inputs->baseOffset;
    task->amplitude = inputs->amplitude;
    task->waveStart = inputs->waveStart;
    task->waveEnd = inputs->waveEnd;
    task->framesPassed = 0;
    task->totalFrames = inputs->totalFrames;
    task->angle = inputs->angle;

    update_sprite_motion_task_sine_osc(task);
    return task;
}


// Sprite Motion - Sinusoidal Velocity to Point (Update)
u32 update_sprite_motion_task_sine_vel(struct SpriteMover_SineVel *task) {
    s32 temp_r5 = lerp(task->unkA, task->unkC, task->framesPassed, task->totalFrames);

    u16 xPos = task->startXPos + FIXED_TO_INT(task->dx * func_080019a4(temp_r5));
    u16 yPos = task->startYPos + FIXED_TO_INT(task->dy * func_080019a4(temp_r5));

    sprite_set_x_y(gSpriteHandler, task->sprite, xPos, yPos);

    return (++task->framesPassed > task->totalFrames);
}


// Sprite Motion - Sinusoidal Velocity to Point (Start)
struct SpriteMover_SineVel *start_sprite_motion_task_sine_vel(struct SpriteMover_SineVel_Inputs *inputs) {
    struct SpriteMover_SineVel *task;

    if (inputs->sprite < 0) {
        return TASK_FAILED_TO_START;
    }

    task = mem_heap_alloc(sizeof(struct SpriteMover_SineVel));
    task->sprite = inputs->sprite;
    task->startXPos = inputs->startX;
    task->startYPos = inputs->startY;
    task->dx = inputs->destX - inputs->startX;
    task->dy = inputs->destY - inputs->startY;
    task->unkA = inputs->unkA * 16;
    task->unkC = inputs->unkC * 16;
    task->framesPassed = 0;
    task->totalFrames = inputs->totalFrames;

    update_sprite_motion_task_sine_vel(task);
    return task;
}


// Sprite Motion - Move Along Sine Wave (Update)
u32 update_sprite_motion_task_sine_wave(struct SpriteMover_SineWave *task) {
    s32 totalFrames = task->totalFrames;
    u32 reachedEnd = (++task->framesPassed >= totalFrames);
    s32 framesPassed = task->framesPassed;
    // Lerp from 0 to pi in terms of sine angle
    s32 temp_r8 = lerp(0, 0x800, framesPassed, totalFrames);

    s16 x = task->startX + lerp(0, task->dx, framesPassed, totalFrames);
    s16 y = task->startY + lerp(0, task->dy, framesPassed, totalFrames) - FIXED_TO_INT(func_080019a4(temp_r8) * task->amplitude);

    sprite_set_x_y(gSpriteHandler, task->sprite, x, y);
    return reachedEnd;
}


// Sprite Motion - Move Along Sine Wave (Start)
struct SpriteMover_SineWave *start_sprite_motion_task_sine_wave(struct SpriteMover_SineWave_Inputs *inputs) {
    struct SpriteMover_SineWave *task;

    if (inputs->sprite < 0) {
        return TASK_FAILED_TO_START;
    }

    task = mem_heap_alloc(sizeof(struct SpriteMover_SineWave));
    task->sprite = inputs->sprite;
    task->startX = inputs->startX;
    task->startY = inputs->startY;
    task->dx = inputs->destX - inputs->startX;
    task->dy = inputs->destY - inputs->startY;
    task->amplitude = inputs->amplitude;
    task->framesPassed = 0;
    task->totalFrames = inputs->totalFrames;

    sprite_set_x_y(gSpriteHandler, task->sprite, task->startX, task->startY);
    return task;
}


// Delete Text Generated by BitmapFontOBJ
void delete_bmp_font_obj_text_anim(struct BitmapFontOBJ *bmpFontOBJ, s16 sprite) {
    if (sprite < 0) {
        return;
    }

    bmp_font_obj_delete_printed_anim(bmpFontOBJ, (struct Animation *)sprite_get_data(gSpriteHandler, sprite, 7));
}


// Delete Text and Sprite Generated by BitmapFontOBJ
void delete_bmp_font_obj_text_sprite(struct BitmapFontOBJ *bmpFontOBJ, s16 sprite) {
    delete_bmp_font_obj_text_anim(bmpFontOBJ, sprite);
    sprite_delete(gSpriteHandler, sprite);
}


/* FAST DIVISION */


#define FAST_UDIVSI3_SIZE ((u32)&fast_udivsi3_rom_end - (u32)fast_udivsi3_rom)

extern u32 fast_udivsi3_rom();
extern void *fast_udivsi3_rom_end;
extern u32 (*fast_udivsi3)(u32 dividend, u32 divisor);

static s32 fast_udivsi3_code[136];


// Init.
void init_fast_udivsi3(void) {
#ifndef PLATFORM_PC
    DmaCopy32(3, fast_udivsi3_rom, fast_udivsi3_code, FAST_UDIVSI3_SIZE);
    fast_udivsi3 = (void *)fast_udivsi3_code;
#else
    extern u32 pc_fast_udivsi3_impl(u32 a, u32 b);
    fast_udivsi3 = pc_fast_udivsi3_impl;
#endif
}


// Divides two signed integers using a fast algorithm contained in IWRAM.
// Much quicker than the versions contained in libc and libagbsyscall.
s32 fast_divsi3(s32 dividend, s32 divisor) {
#ifdef PLATFORM_PC
    if (divisor == 0) return 0;
    return dividend / divisor;
#else
    u32 quotient;
    u32 isNegative = FALSE;

    if (dividend & 0x80000000) {
        dividend = -dividend;
        isNegative = TRUE;
    }

    if (divisor & 0x80000000) {
        divisor = -divisor;
        isNegative ^= TRUE;
    }

    quotient = fast_udivsi3(dividend, divisor);
    return isNegative ? -quotient : quotient;
#endif
}


/* INTERPOLATION */


// Initialise Number Linear Interpolator Task
struct NumberInterpolator *start_integer_interp_task(struct NumberInterpolator *inputs) {
    struct NumberInterpolator *task;

    task = mem_heap_alloc(sizeof(struct NumberInterpolator));
    task->type = inputs->type;
    task->duration = inputs->duration;
    task->runningTime = 0;
    task->source = inputs->source;
    task->initial = inputs->initial;
    task->target = inputs->target;

    switch (task->type) {
        case 0:
            *(u8 *)task->source = task->initial;
            break;
        case 1:
            *(u16 *)task->source = task->initial;
            break;
        case 2:
            *(u32 *)task->source = task->initial;
            break;
    }

    return task;
}


// Update Number Linear Interpolator Task
u32 update_integer_interp_task(struct NumberInterpolator *task) {
    s32 current = task->initial + fast_divsi3((task->target - task->initial) * task->runningTime, task->duration);

    switch (task->type) {
        case 0:
            *(u8 *)task->source = current;
            break;
        case 1:
            *(u16 *)task->source = current;
            break;
        case 2:
            *(u32 *)task->source = current;
            break;
    }

    return (++task->runningTime > task->duration);
}


// Initialise Number Alternator Task
struct NumberInterpolator *start_integer_alternator_task(struct NumberInterpolator *inputs) {
    struct NumberInterpolator *task;

    task = mem_heap_alloc(sizeof(struct NumberInterpolator));
    task->type = inputs->type;
    task->duration = inputs->duration;
    task->runningTime = 0;
    task->source = inputs->source;
    task->initial = inputs->initial;
    task->target = inputs->target;

    switch (task->type) {
        case 0:
            *(u8 *)task->source = task->initial;
            break;
        case 1:
            *(u16 *)task->source = task->initial;
            break;
        case 2:
            *(u32 *)task->source = task->initial;
            break;
    }

    return task;
}


// Update Number Alternator Task
u32 update_integer_alternator_task(struct NumberInterpolator *task) {
    s32 newInitial, newTarget;

    if (++task->runningTime >= task->duration) {
        task->runningTime = 0;
        newInitial = task->target;
        newTarget = task->initial;
        task->target = newTarget;
        task->initial = newInitial;

        switch (task->type) {
            case 0:
                *(u8 *)task->source = newInitial;
                break;
            case 1:
                *(u16 *)task->source = newInitial;
                break;
            case 2:
                *(u32 *)task->source = newInitial;
                break;
        }
    }

    return FALSE;
}


// Initialise Number Incrementer Task
struct NumberInterpolator *start_integer_incrementer_task(struct NumberInterpolator *inputs) {
    struct NumberInterpolator *task;

    task = mem_heap_alloc(sizeof(struct NumberInterpolator));
    task->type = inputs->type;
    task->duration = inputs->duration;
    task->runningTime = 0;
    task->source = inputs->source;
    task->initial = inputs->initial;
    task->target = inputs->target;

    switch (task->type) {
        case 0:
            *(u8 *)task->source = task->initial;
            break;
        case 1:
            *(u16 *)task->source = task->initial;
            break;
        case 2:
            *(u32 *)task->source = task->initial;
            break;
    }

    return task;
}


// Update Number Incrementer Task
u32 update_integer_incrementer_task(struct NumberInterpolator *task) {
    s32 current;

    if (++task->runningTime >= task->duration) {
        task->runningTime = 0;
        current = task->initial + task->target;
        task->initial = current;

        switch (task->type) {
            case 0:
                *(u8 *)task->source = current;
                break;
            case 1:
                *(u16 *)task->source = current;
                break;
            case 2:
                *(u32 *)task->source = current;
                break;
        }
    }

    return FALSE;
}


// Set Target Value for Number Task
void set_target_for_integer_task(s32 taskID, s32 newTarget) {
    struct NumberInterpolator *task = get_task_info(taskID);

    if (task != NULL) {
        task->target = newTarget;
    }
}


// Initialise Number Sine Interpolator Task
struct NumberSineInterpolator *start_integer_sine_interp_task(struct NumberSineInterpolator *inputs) {
    struct NumberSineInterpolator *task;
    s32 current;

    task = mem_heap_alloc(sizeof(struct NumberSineInterpolator));
    task->type = inputs->type;
    task->angle = inputs->angle;
    task->speed = inputs->speed;
    task->value = inputs->value;
    task->source = inputs->source;

    current = FIXED_POINT_MUL(sins(FIXED_TO_INT(task->angle)), task->value);

    switch (task->type) {
        case 0:
            *(u8 *)task->source = current;
            break;
        case 1:
            *(u16 *)task->source = current;
            break;
        case 2:
            *(u32 *)task->source = current;
            break;
    }

    return task;
}


// Update Number Sine Interpolator Task
u32 update_integer_sine_interp_task(struct NumberSineInterpolator *task) {
    s32 current;

    task->angle += task->speed;
    current = FIXED_POINT_MUL(sins(FIXED_TO_INT(task->angle)), task->value);

    switch (task->type) {
        case 0:
            *(u8 *)task->source = current;
            break;
        case 1:
            *(u16 *)task->source = current;
            break;
        case 2:
            *(u32 *)task->source = current;
            break;
    }

    return FALSE;
}


// Initialise LCD Special Effects Interpolator
struct BlendControlsInterpolator *start_lcd_blend_mode_interp(struct BlendControlsInterpolator *inputs) {
    struct BlendControlsInterpolator *task;
    u32 tgtLevel, srcLevel;

    task = mem_heap_alloc(sizeof(struct BlendControlsInterpolator));
    task->blendControls = inputs->blendControls;
    task->duration = inputs->duration;
    task->runningTime = 0;
    task->flip = inputs->flip;

    tgtLevel = 0x10 * (inputs->flip != 0);
    srcLevel = 0x10 - tgtLevel;
    switch (task->blendControls & BLDMOD_BLEND_MODE_MASK) {
        case (BLEND_MODE_OFF * 0x40):
        case (BLEND_MODE_ALPHA * 0x40):
            D_03004b10.COLEV = (srcLevel | (tgtLevel << 8));
            break;

        case (BLEND_MODE_LIGHTEN * 0x40):
        case (BLEND_MODE_DARKEN * 0x40):
            D_03004b10.COLEY = tgtLevel;
            break;
    }

    D_03004b10.BLDMOD = task->blendControls;
    return task;
}


// Update LCD Special Effects Interpolator
u32 update_lcd_blend_mode_interp(struct BlendControlsInterpolator *task) {
    u32 tgtLevel, srcLevel;

    task->runningTime++;
    tgtLevel = Div(task->runningTime * 0x10, task->duration);

    if (task->flip) {
        srcLevel = tgtLevel;
        tgtLevel = 0x10 - tgtLevel;
    } else {
        srcLevel = 0x10 - tgtLevel;
    }

    switch (task->blendControls & BLDMOD_BLEND_MODE_MASK) {
        case (BLEND_MODE_OFF * 0x40):
        case (BLEND_MODE_ALPHA * 0x40):
            D_03004b10.COLEV = (srcLevel | (tgtLevel << 8));
            break;

        case (BLEND_MODE_LIGHTEN * 0x40):
        case (BLEND_MODE_DARKEN * 0x40):
            D_03004b10.COLEY = tgtLevel;
            break;
    }

    if (task->runningTime < task->duration) {
        return FALSE;
    } else {
        return TRUE;
    }
}


// Interpolate LCD Special Effects
s32 interp_lcd_blend_mode(u16 memID, u32 blendControls, u32 duration, u32 flip) {
    struct BlendControlsInterpolator inputs;

    inputs.blendControls = blendControls;
    inputs.duration = duration;
    inputs.flip = flip;

    return start_new_task(memID, &lcd_blend_mode_interp_task, &inputs, NULL, 0);
}


// Interpolate Window Size
void interp_screen_window_size(u16 memID, u32 window, u32 duration,
                                s32 initialX1, s32 initialY1, s32 initialX2, s32 initialY2,
                                s32 targetX1, s32 targetY1, s32 targetX2, s32 targetY2) {
    struct NumberInterpolator inputs;
    void *windowLeftEdge;
    void *windowRightEdge;
    void *windowTopEdge;
    void *windowBottomEdge;

    inputs.type = 0;
    inputs.duration = duration;

    windowRightEdge = (window != 0) ? &D_03004b10.WIN1H : &D_03004b10.WIN0H;
    windowBottomEdge = (window != 0) ? &D_03004b10.WIN1V : &D_03004b10.WIN0V;
    windowLeftEdge = windowRightEdge + 1;
    windowTopEdge = windowBottomEdge + 1;

    inputs.source = windowLeftEdge;
    inputs.initial = initialX1;
    inputs.target = targetX1;
    start_new_task(memID, &integer_interp_task, &inputs, NULL, 0);

    inputs.source = windowTopEdge;
    inputs.initial = initialY1;
    inputs.target = targetY1;
    start_new_task(memID, &integer_interp_task, &inputs, NULL, 0);

    inputs.source = windowRightEdge;
    inputs.initial = initialX2;
    inputs.target = targetX2;
    start_new_task(memID, &integer_interp_task, &inputs, NULL, 0);

    inputs.source = windowBottomEdge;
    inputs.initial = initialY2;
    inputs.target = targetY2;
    start_new_task(memID, &integer_interp_task, &inputs, NULL, 0);

    D_03004b10.DISPCNT |= DISPCNT_ENABLE_WINDOW(window);
}


/* STRING */


extern char D_08936c64[]; // "?��O?��P?��Q?��R?��S?��T?��U?��V?��W?��X"


#ifndef PLATFORM_PC
// Copy Substring
char *strncpy(char *s1, const char *s2, u32 len) {
    char *s = s1;
    u32 i;

    for (i = 0; (*s2 != '\0') && (i < len); i++) {
        *s1++ = *s2++;
    }

    return s;
}


// Concatenate String
char *strcat(char *s1, const char *s2) {
    char *s = s1;

    while (*s1 != '\0') {
        s1++;
    }

    while (*s2 != '\0') {
        *s1++ = *s2++;
    }

    *s1 = '\0';
    return s;
}


// Concatenate Substring
char *strncat(char *s1, const char *s2, u32 len) {
    char *s = s1;
    u32 i;

    while (*s1 != '\0') {
        s1++;
    }

    for (i = 0; (*s2 != '\0') && (i < len); i++) {
        *s1++ = *s2++;
    }

    *s1 = '\0';

    return s;
}


// Compare Strings
s32 strncmp(const char *s1, const char *s2, u32 len) {
    u32 i;

    for (i = 0; ((*s1 != '\0') || (*s2 != '\0')) && (i < len); i++) {
        if (*s1 != *s2) {
            return (*s1 > *s2) ? 1 : -1;
        }

        s1++;
        s2++;
    }

    return 0;
}
#endif // !PLATFORM_PC


// Integer to String (Halfwidth)
void strint(char *s, u32 n) {
    u32 len = 1;
    u32 d, i;

    for (d = 10; d <= n; d *= 10) {
        len++;
    }

    s += len;
    *s = '\0';

    for (i = 0; i < len; i++) {
        *(--s) = '0' + (n % 10);
        n /= 10;
    }
}


// Integer to String (Halfwidth)
void strnint(char *s, u32 n, u32 len) {
    u32 i;

    s += len;
    *s = '\0';

    for (i = 0; i < len; i++) {
        *(--s) = '0' + (n % 10);
        n /= 10;
    }
}


// Integer to String (Fullwidth)
void strintf(char *s, u32 n) {
    u32 len = 1;
    u32 d, i;

    for (d = n; d > 9; d /= 10) {
        len++;
    }

    s += (len * 2);
    *s = '\0';

    for (i = 0; i < len; i++) {
        char *c = &D_08936c64[(n % 10) * 2];

        s -= 2;
        s[0] = c[0];
        s[1] = c[1];
        n /= 10;
    }
}


// Integer to String (Fullwidth)
void strnintf(char *s, u32 n, u32 len) {
    u32 i;

    s += (len * 2);
    *s = '\0';

    for (i = 0; i < len; i++) {
        char *c = &D_08936c64[(n % 10) * 2];

        s -= 2;
        s[0] = c[0];
        s[1] = c[1];
        n /= 10;
    }
}


/* ? */


void func_08008370(u32 *arg0, u32 *arg1, u32 arg2, u32 arg3) {
    u32 temp_r1;
    u32 temp;
    u32 temp_r6;
    u32 temp_r7;
    u32 temp_ip;
    u32 i;
    
    temp_ip = (arg2 + 3) >> 2;
    arg0 += temp_ip;
    arg1 += temp_ip * 2;
    temp_r6 = 0x33333333; // constant 1100 bit pattern
    temp_r1 = (arg3 << 0) | (arg3 << 4) | (arg3 << 8) | (arg3 << 0xc); // initial pattern
    temp_r1 |= (temp_r1 << 0x10); // copy into high bytes
    temp_r7 = (arg2 + 3) >> 4;
    for (i = 0; i < temp_r7; i++) {
        arg0 -= 4;
        arg1 -= 4 * 2;

        temp = arg0[3];
        arg1[6] = (temp & temp_r6) + temp_r1;
        arg1[7] = ((temp >> 2) & temp_r6) + temp_r1;
        temp = arg0[2];
        arg1[4] = (temp & temp_r6) + temp_r1;
        arg1[5] = ((temp >> 2) & temp_r6) + temp_r1;
        temp = arg0[1];
        arg1[2] = (temp & temp_r6) + temp_r1;
        arg1[3] = ((temp >> 2) & temp_r6) + temp_r1;
        temp = arg0[0];
        arg1[0] = (temp & temp_r6) + temp_r1;
        arg1[1] = ((temp >> 2) & temp_r6) + temp_r1;
    }
    for (i = 0; i < temp_ip; i++) {
        arg0 -= 1;
        arg1 -= 1 * 2;

        temp = arg0[0];
        arg1[0] = (temp & temp_r6) + temp_r1;
        arg1[1] = ((temp >> 2) & temp_r6) + temp_r1;
    }
}


// D_08936c7c function 1
struct unk_struct_08008420 *func_08008420(struct unk_struct_08008420_init *inputs) {
    struct unk_struct_08008420 *task;
    u32 temp_r3;

    task = mem_heap_alloc(sizeof(struct unk_struct_08008420));
    task->unk8 = (inputs->unk8 + 3) >> 2;
    task->unkC = (inputs->unkC + 3) >> 2;
    task->unk0 = &inputs->unk0[task->unk8];
    task->unk4 = &inputs->unk4[task->unk8 * 2];
    temp_r3 = inputs->unk10;
    temp_r3 = (temp_r3 << 0) | (temp_r3 << 4) | (temp_r3 << 8) | (temp_r3 << 0xc);
    temp_r3 |= (temp_r3 << 0x10);
    task->unk10 = temp_r3;
    return task;
}

// D_08936c7c function 2
u32 func_08008464(struct unk_struct_08008420 *task) {
    u32 *temp_r4 = task->unk0;
    u32 *temp_r2 = task->unk4;
    u32 temp_r5 = 0x33333333;
    u32 temp_r3 = task->unk10;
    u32 temp;
    u32 i;
    u32 temp_r8 = (task->unkC > task->unk8) ? task->unk8 : task->unkC;
    u32 temp_ip = temp_r8 >> 2;
    
    for (i = 0; i < temp_ip; i++) {
        temp_r4 -= 4;
        temp_r2 -= 4 * 2;

        temp = temp_r4[3];
        temp_r2[6] = (temp & temp_r5) + temp_r3;
        temp_r2[7] = ((temp >> 2) & temp_r5) + temp_r3;
        temp = temp_r4[2];
        temp_r2[4] = (temp & temp_r5) + temp_r3;
        temp_r2[5] = ((temp >> 2) & temp_r5) + temp_r3;
        temp = temp_r4[1];
        temp_r2[2] = (temp & temp_r5) + temp_r3;
        temp_r2[3] = ((temp >> 2) & temp_r5) + temp_r3;
        temp = temp_r4[0];
        temp_r2[0] = (temp & temp_r5) + temp_r3;
        temp_r2[1] = ((temp >> 2) & temp_r5) + temp_r3;
    }
    for (i = 0; i < (temp_r8 & 3); i++) {
        temp_r4 -= 1;
        temp_r2 -= 1 * 2;

        temp = temp_r4[0];
        temp_r2[0] = (temp & temp_r5) + temp_r3;
        temp_r2[1] = ((temp >> 2) & temp_r5) + temp_r3;
    }
    
    task->unk0 = temp_r4;
    task->unk4 = temp_r2;
    task->unk8 -= temp_r8;
    
    if (task->unk8 > 0) {
        return FALSE;
    } else {
        return TRUE;
    }
}


/* SCHEDULED FUNCTION CALL */


// Initialise Scheduled Function Task
struct ScheduledFunctionTask *init_scheduled_function_task(struct ScheduledFunctionTask *inputs) {
    struct ScheduledFunctionTask *task;

    task = mem_heap_alloc(sizeof(struct ScheduledFunctionTask));
    task->function = inputs->function;
    task->param = inputs->param;
    task->delay = inputs->delay;
    return task;
}

// Update Scheduled Function Task
u32 update_scheduled_function_task(struct ScheduledFunctionTask *task) {
    if (task->delay) {
        task->delay--;
        return FALSE;
    }
    if (task->function) {
        task->function(task->param);
    }
    return TRUE;
}

// Scheduled Function Call
s32 schedule_function_call(u16 memID, void *function, intptr_t param, u32 delay) {
    struct ScheduledFunctionTask inputs;

    inputs.function = function;
    inputs.param = param;
    inputs.delay = delay;
    return start_new_task(memID, &delayed_function_call_task, &inputs, NULL, 0);
}


/* BUFFERED TEXTURE */

static struct GFXDecompressProgress D_030010d0;
extern u32 decompress_gfx_rom(struct GFXDecompressProgress *);

u32 decompress_gfx_init(struct CompressedGFX *gfx, uintptr_t dest, u32 limit, struct GFXDecompressProgress *progress) {
    u32 (*decompress_gfx)(struct GFXDecompressProgress *);

    if (!progress) {
        progress = &D_030010d0;
    }
    progress->data = gfx->data;
    progress->size = dest;
    progress->count = ((gfx->count - 1) << 16) | 0x2020;
    progress->curwin1 = *gfx->window1;
    progress->curwin2 = *gfx->window2;
    progress->win1 = gfx->window1 + 1;
    progress->win2 = gfx->window2 + 1;
    progress->lim1 = limit;
    progress->lim2 = limit;

    decompress_gfx = decompress_gfx_rom;
    return decompress_gfx(progress);
}

u32 decompress_gfx_resume(struct GFXDecompressProgress *progress) {
    u32 (*decompress_gfx)(struct GFXDecompressProgress *);

    if (!progress) {
        progress = &D_030010d0;
    }
    progress->lim1 = progress->lim2;

    decompress_gfx = decompress_gfx_rom;
    return decompress_gfx(progress);
}

#ifndef PLATFORM_PC
#include "asm/code_08007468/asm_08008608.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08007468/asm_0800861c.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08007468/asm_08008628.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08007468/asm_08008658.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08007468/asm_0800869c.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08007468/asm_080086c4.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08007468/asm_08008720.s"
#endif

// D_08936c9c function 1
#ifndef PLATFORM_PC
#include "asm/code_08007468/asm_0800873c.s"
#endif

// D_08936c9c function 2
#ifndef PLATFORM_PC
#include "asm/code_08007468/asm_08008758.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08007468/asm_080087b4.s"
#endif
#ifdef PLATFORM_PC
/* ── Buffered textures / texture loader (C translation) ─────────────────────
 * This whole cluster was assembly, so on PC every entry point was an
 * auto_stub returning 0.  Two consequences:
 *   - start_new_texture_loader registered no task, so engines that load their
 *     graphics through the loader (rather than the beatscript's load_graphics
 *     opcode) never got them, and drew a flat fill;
 *   - func_0800869c returned NULL, and graphics_table.c dereferences its
 *     result immediately.
 *
 * From asm/code_08007468/{asm_08008608, asm_0800861c, asm_08008628,
 * asm_08008658, asm_0800869c, asm_080086c4, asm_08008720, asm_0800873c,
 * asm_08008758, asm_080087b4}.s.
 */

// One texture that has had its GFX layer expanded into a heap buffer, so a
// texture shared by several engines is only decompressed once.  Singly linked.
//
// `view` is not bookkeeping: func_0800869c hands its address back to the
// graphics-table loader as a CompressedData, describing the same texture with
// the expansion already done (doubleCompressed cleared, so only the RLE stage
// is left).  On GBA the two are literally overlaid at entry+4; keeping a real
// struct here says the same thing without depending on field offsets.
struct BufferedTextureEntry *D_0300536c;

// [func_080086c4] Find `src` in the cache, or add an entry with a buffer big
// enough for it.  Returns NULL when the texture needs no buffering, or is
// already present — i.e. non-NULL means "now go and fill this one in".
struct BufferedTextureEntry *func_080086c4(const struct CompressedData *src) {
    struct BufferedTextureEntry *e;

    if (src->doubleCompressed == 0) {
        return NULL;
    }
    for (e = D_0300536c; e != NULL; e = e->next) {
        if (e->src == src) {
            return NULL;                 // already buffered
        }
    }

    e = mem_heap_alloc(sizeof(struct BufferedTextureEntry));
    e->src  = src;
    e->next = D_0300536c;
    D_0300536c = e;

    // `size` is the expanded length in halfwords.  (`count` is the number of
    // compression units, which is smaller — using it undersizes the buffer.)
    e->data.data = mem_heap_alloc(((struct CompressedGFX *)src->data)->size * 2);
    e->data.rleData          = src->rleData;
    e->data.rleSize          = src->rleSize;
    e->data.rleOffset        = src->rleOffset;
    e->data.doubleCompressed = 0;
    return e;
}

// [func_0800861c] Drop the whole cache without freeing — for when the heap it
// lives in is being reset wholesale.
void func_0800861c(void) {
    D_0300536c = NULL;
}

// [func_08008628] Free every buffered texture and its buffer.
void func_08008628(void) {
    struct BufferedTextureEntry *e = D_0300536c;

    while (e != NULL) {
        struct BufferedTextureEntry *next = e->next;
        mem_heap_dealloc((void *)e->data.data);
        mem_heap_dealloc(e);
        e = next;
    }
    D_0300536c = NULL;
}

// [func_08008658] Drop one texture from the cache.
void func_08008658(const struct CompressedData *src) {
    struct BufferedTextureEntry *e = D_0300536c;
    struct BufferedTextureEntry *prev = NULL;

    while (e != NULL) {
        if (e->src == src) {
            if (prev == NULL) {
                D_0300536c = e->next;
            } else {
                prev->next = e->next;
            }
            mem_heap_dealloc((void *)e->data.data);
            mem_heap_dealloc(e);
            return;
        }
        prev = e;
        e = e->next;
    }
}

// [func_0800869c] Substitute the buffered form of `src` if there is one.
struct CompressedData *func_0800869c(const struct CompressedData *src) {
    struct BufferedTextureEntry *e;

    for (e = D_0300536c; e != NULL; e = e->next) {
        if (e->src == src) {
            return &e->data;
        }
    }
    return (struct CompressedData *)src;
}

// [func_08008608] Expand one texture in a single call (no frame budget, and no
// progress block — decompress_gfx_init falls back to its own static one).
u32 func_08008608(struct CompressedGFX *gfx, void *dest) {
    return decompress_gfx_init(gfx, (uintptr_t)dest, 0x7fffffff, NULL);
}

// [func_08008720] Buffer one texture immediately, if it isn't already.
void func_08008720(const struct CompressedData *src) {
    struct BufferedTextureEntry *e = func_080086c4(src);
    if (e != NULL) {
        func_08008608((struct CompressedGFX *)src->data, (void *)e->data.data);
    }
}

// The loader task's own state.  On GBA a 0x2C-byte block: list cursor at +0,
// decompression progress at +4, busy flag at +0x28.
struct TextureLoader {
    struct CompressedData **list;
    struct GFXDecompressProgress progress;
    u8 busy;
};

// [init_texture_loader_task] D_08936c9c function 1
struct TextureLoader *init_texture_loader_task(struct TextureLoaderInputs *inputs) {
    struct TextureLoader *st = mem_heap_alloc(sizeof(struct TextureLoader));
    st->list = (struct CompressedData **)inputs;
    st->busy = 0;
    return st;
}

// [update_texture_loader_task] D_08936c9c function 2.
// Returns non-zero once the whole list has been expanded.  Each call does at
// most 0x1000 units of work and resumes where it left off, so a long list is
// spread over frames instead of stalling one.
u32 update_texture_loader_task(struct TextureLoader *st) {
    u32 finished;

    if (st->busy) {
        finished = decompress_gfx_resume(&st->progress);
    } else {
        for (;;) {
            struct CompressedData *src = *st->list++;
            struct BufferedTextureEntry *e;

            if (src == NULL) {
                return 1;                // end of list
            }
            e = func_080086c4(src);
            if (e == NULL) {
                continue;                // already buffered, or not compressed
            }
            finished = decompress_gfx_init((struct CompressedGFX *)src->data,
                                           (uintptr_t)e->data.data,
                                           0x1000, &st->progress);
            st->busy = 1;
            break;
        }
    }

    if (finished != 0) {
        st->busy = 0;                    // this texture is done; next call takes the next one
    }
    return 0;
}

// [start_new_texture_loader] Register the loader as a pool task.
u32 start_new_texture_loader(u16 memID, struct CompressedData **textureList) {
    return start_new_task(memID, &D_08936c9c, textureList, NULL, 0);
}
#endif



/* ? */


// Clamp Signed Integer
s32 clamp_int32(s32 var, s32 min, s32 max) {
    if (var < min) {
        return min;
    }
    if (var > max) {
        return max;
    }
    return var;
}


#ifndef PLATFORM_PC
#include "asm/code_08007468/asm_080087e8.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_080087e8] Scale and rotate an affine BG
//
// Fits the 240x160 screen to a width x height window (8.8 fixed) rotated by
// `rotation`, turning about (centerX, centerY), with the window's centre at
// (x, y). mode 0 takes the angle from the 256-step tables, anything else
// from gSineTable (2048 steps). Only BG2 and BG3 can be affine.
void func_080087e8(s32 layer, s32 x, s32 y, s32 width, s32 height, s32 rotation,
                   u16 centerX, u16 centerY, s32 mode) {
    s32 sin, cos;
    s32 offX, offY;
    u16 pa, pb, pc, pd;

    if ((u32)(layer - 2) > 1) return;

    if (mode == 0) {
        sin = D_08935fcc[rotation & 0xff];
        cos = D_089361cc[rotation & 0xff];
    } else {
        sin = gSineTable[rotation & 0x7ff];
        cos = gSineTable[(rotation + 0x200) & 0x7ff];
    }

    offX = -fast_divsi3((s16)centerX * width, 0xf0);
    offY = -fast_divsi3((s16)centerY * height, 0xa0);

    pa = fast_divsi3((width * cos) >> 8, 0xf0);
    pc = fast_divsi3((width * sin) >> 8, 0xf0);
    pb = -fast_divsi3((height * sin) >> 8, 0xa0);
    pd = fast_divsi3((height * cos) >> 8, 0xa0);

    func_08006e30(layer, pa, pb, pc, pd,
                  (((offX * cos) - (offY * sin)) >> 8) + x,
                  (((offX * sin) + (offY * cos)) >> 8) + y);
}
#endif

#ifndef PLATFORM_PC
#include "asm/code_08007468/asm_08008910.s"
#else
// Translated from the assembly above and checked against it; not proven byte-exact.
// [func_08008910] Scale and rotate an affine BG about the screen centre
void func_08008910(s32 layer, s32 x, s32 y, s32 width, s32 height, u32 rotation) {
    func_080087e8(layer, x, y, width, height, (u8)rotation, 0x78, 0x50, 0);
}
#endif

#ifndef PLATFORM_PC
#include "asm/code_08007468/asm_08008938.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08007468/asm_08008968.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08007468/asm_08008990.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08007468/asm_080089c0.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08007468/asm_08008a70.s"
#endif

#ifndef PLATFORM_PC
#include "asm/code_08007468/asm_08008ab8.s"
#endif

void func_08008b00(u32 unused_arg0, u32 unused_arg1, s16 arg2, s24_8 arg3, s24_8 arg4,
    s16 arg5, s24_8 arg6, s24_8 arg7, u16 arg8,
    struct unk_struct_08008b00 *arg9, u32 arg10, u32 arg11) {

    s24_8 temp_sp0, temp_sp4, temp_r9, temp_r8;
    s32 ret_r5, ret_sp8;
    s24_8 temp_r7, ret_spc, ret_sp10;
    s32 ret_sp14;
    s24_8 temp_sp18, temp_sp1c, temp_sp20;
    s32 i;
    s24_8 temp_8000 = INT_TO_FIXED(128);

    temp_sp0 = arg11 ? sins(arg5) : sins2(arg5);
    temp_sp4 = arg11 ? coss(arg5) : coss2(arg5);

    temp_r9 = arg11 ? sins(-arg2) : sins2(-arg2);
    temp_r8 = arg11 ? coss(-arg2) : coss2(-arg2);

    ret_r5 = fast_divsi3(160 * 256, arg8);
    ret_sp14 = fast_divsi3(160 * 256, arg8);
    ret_sp8 = fast_divsi3(ret_r5 * 240, 160);

    temp_r7 = INT_TO_FIXED(-ret_r5) >> 1;

    ret_spc = fast_divsi3(INT_TO_FIXED(ret_r5), 160);
    temp_r7 += fast_divsi3(ret_r5 * arg7, 160);
    arg3 += fast_divsi3(ret_r5 * arg6, 160);
    ret_sp10 = fast_divsi3(INT_TO_FIXED(ret_r5), 160);

    temp_sp18 = ret_sp14 * temp_sp0;

    temp_sp1c = arg3 - temp_8000;
    temp_sp20 = arg4 - temp_8000;

    for (i = 0; i < 160; i++) {
        s24_8 temp_r1;
        s32 temp_r5_2 = FALSE;
        if (temp_sp0 != 0) {
            temp_r1 = FIXED_POINT_MUL(temp_r7, temp_sp4) - temp_sp18;
            if (temp_r1 == 0) {
                temp_r5_2 = TRUE;
            }
        }
        if ((temp_sp0 == 0) || temp_r5_2) {
            arg9->unk8 = -1;
            arg9->unkC = -1;
            arg9->unk6 = 0;
            arg9->unk4 = 0;
            arg9->unk2 = 0;
            arg9->unk0 = 0;
        } else {
            s24_8 m_r4, m_r3, m_r2;
            temp_r1 = fast_divsi3(INT_TO_FIXED(ret_sp14) << 8, temp_r1); // fixed point division
            m_r4 = FIXED_POINT_MUL(-temp_r1, temp_sp0);

            m_r3 = temp_sp1c - (ret_sp8 >> 1) * m_r4;
            m_r2 = temp_sp20 + FIXED_POINT_MUL(temp_r1, temp_r7);

            arg9->unk8 = ((temp_r8 * m_r3 - temp_r9 * m_r2) >> 8) + temp_8000; // doesn't work with the macros
            arg9->unkC = ((temp_r9 * m_r3 + temp_r8 * m_r2) >> 8) + temp_8000;
            
            m_r4 *= ret_sp10;
            arg9->unk0 = FIXED_POINT_MUL(temp_r8, m_r4) >> 0x8;
            arg9->unk2 = 0;
            arg9->unk4 = FIXED_POINT_MUL(temp_r9, m_r4) >> 0x8;
            arg9->unk6 = 0;
        }
        temp_r7 += ret_spc;
        arg9 += arg10;
    }
}

void func_08008d44(u32 unused_arg0, u32 unused_arg1, s16 arg2, s24_8 arg3, s24_8 arg4, s16 arg5, s24_8 arg6, s24_8 arg7, u16 arg8, struct unk_struct_08008b00 *arg9, u32 arg10) {
    func_08008b00(unused_arg0, unused_arg1, arg2, arg3, arg4, arg5 - 0x200, arg6, arg7, arg8, arg9, arg10, 0);
}

void func_08008d88(u32 unused_arg0, u32 unused_arg1, s16 arg2, s24_8 arg3, s24_8 arg4, s16 arg5, s24_8 arg6, s24_8 arg7, u16 arg8, struct unk_struct_08008b00 *arg9, u32 arg10) {
    func_08008b00(unused_arg0, unused_arg1, arg2, arg3, arg4, arg5 - 0x200, arg6, arg7, arg8, arg9, arg10, 1);
}
