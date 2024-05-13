/*
Copyright 2024 Tomoaki Itoh
This software is released under the MIT License, see LICENSE.txt.
//*/

#include "userdelfx.h"

#include <climits>
#include "buffer_ops.h"
#include "LCWDelay.h"
#include "LCWDelayParam.h"

#define param_10bit_to_8bit(val) (val >> 2) // 0..0x3FF -> 0..0xFF
#define param_10bit_to_6bit(val) (val >> 4) // 0..0x3FF -> 0..0x3F
#define param_10bit_to_4bit(val) (val >> 6) // 0..0x3FF -> 0..0x0F

static struct {
    int32_t time = 0;
    int32_t depth = 0;
    float mix = 0.f;
} s_param;

static __sdram float delay_ram_sampling[LCW_DELAY_SAMPLING_SIZE];
static __sdram float delay_ram_input[LCW_DELAY_IO_BUFFER_SIZE];

static float s_inputGain;
static LCWDelayBuffer delayLine;
static LCWDelayBuffer inputBuffer;
static LCWDelayBlock delayBlock;

void DELFX_INIT(uint32_t platform, uint32_t api)
{
    // set default values
    s_param.time = 0;
    s_param.depth = 0;
    s_param.mix = 0.f;
    s_inputGain = 0.f;

    delayLine.buffer = delay_ram_sampling;
    delayLine.size = LCW_DELAY_SAMPLING_SIZE;
    delayLine.mask = LCW_DELAY_SAMPLING_SIZE - 1;
    delayLine.pointer = 0;

    inputBuffer.buffer = delay_ram_input;
    inputBuffer.size = LCW_DELAY_IO_BUFFER_SIZE;
    inputBuffer.mask = LCW_DELAY_IO_BUFFER_SIZE - 1;
    inputBuffer.pointer = 0;

    delayBlock.delayLine = &delayLine;
    delayBlock.inputBuffer = &inputBuffer;
    delayBlock.param.fbGain = 0.f;
    delayBlock.param.position = 0.f;
    delayBlock.currentPosition = 0.f;

    LCWDelayInit(&delayBlock);
}

#define LCW_DELAY_SAMPLING_RATE (48000)
void DELFX_PROCESS(float *xn, uint32_t frames)
{
    const int32_t delayTime = param_10bit_to_8bit(s_param.time);
    delayBlock.param.position = (float)delayTimeParams[delayTime];

    const int32_t depthIndex = param_10bit_to_6bit(s_param.depth);
    delayBlock.param.fbGain = -1 * delayFbGainTable[depthIndex]; // memo: 符号反転はお好み

    LCWDelayUpdate(&delayBlock);

    float * __restrict x = xn;
    const float * x_e = x + 2*frames;

    const float dry = 1.f - s_param.mix;
    const float wet = s_param.mix;

    // 切り替え時のノイズ対策
    if ( s_inputGain < 0.99998f ) {
        for (; x != x_e; ) {
            *(x++) = *x * s_inputGain;
            *(x++) = *x * s_inputGain;

            if ( s_inputGain < 0.99998f ) {
            s_inputGain += ( (1.f - s_inputGain) * 0.0625f );
            }
            else {
                s_inputGain = 1.f;
                break;
            }
        }
    }

    x = xn;
    for (; x != x_e;) {
        const float xL = *(x + 0);
        // const float xR = *(x + 1);

        const float wL = LCWDelayProcess(&delayBlock, xL);
        // const float wR = xR;

        const float y = (dry * xL) + (wet * wL);
        *(x++) = y;
        *(x++) = y;
        // *(x++) = (dry * xR) + (wet * wR);
    }
}

void DELFX_RESUME(void)
{
    s_inputGain = 0.f;
}

void DELFX_PARAM(uint8_t index, int32_t value)
{
    const float valf = q31_to_f32(value);
    switch (index) {
    case k_user_delfx_param_time:
        s_param.time = clipminmaxi32(0, (int32_t)(valf * 1024.f), 1023);
        break;
    case k_user_delfx_param_depth:
        s_param.depth = clipminmaxi32(0, (int32_t)(valf * 1024.f), 1023);
        break;
    case k_user_delfx_param_shift_depth:
        // Rescale to add notch around 0.5f
        s_param.mix = (valf <= 0.49f) ? 1.02040816326530612244f * valf : (valf >= 0.51f) ? 0.5f + 1.02f * (valf-0.51f) : 0.5f;
        break;
    default:
        break;
    }
}
