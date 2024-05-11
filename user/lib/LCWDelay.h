/*
Copyright 2024 Tomoaki Itoh
This software is released under the MIT License, see LICENSE.txt.
//*/

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LCW_DELAY_SAMPLING_SIZE (0x20000)
#define LCW_DELAY_IO_BUFFER_SIZE (0x100)

#define LCW_DELAY_BUFFER_DEC(p) ( ((p)->pointer - 1) & (p)->mask )
#define LCW_DELAY_BUFFER_LUT(p, i) ( (p)->buffer[((p)->pointer + (i)) & (p)->mask] )

typedef struct {
    float *buffer;
    uint32_t size;
    uint32_t mask;
    int32_t pointer;
} LCWDelayBuffer;

typedef struct {
    float fbGain;
    float position;
    int32_t clipIndex;
} LCWDelayParam;

typedef struct {
    const float *param;
    int32_t num;
} LCWFirParam;

typedef struct {
    LCWDelayBuffer *inputBuffer;
    LCWDelayBuffer *delayLine;
    LCWDelayParam param;
    LCWFirParam hpf;
    float currentPosition;
} LCWDelayBlock;

extern void LCWDelayInit(LCWDelayBlock *block);
extern void LCWDelayUpdate(LCWDelayBlock *block);
extern float LCWDelayProcess(LCWDelayBlock *block, float input);

#ifdef __cplusplus
}
#endif
