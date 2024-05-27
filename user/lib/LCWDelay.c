/*
Copyright 2024 Tomoaki Itoh
This software is released under the MIT License, see LICENSE.txt.
//*/

#include "LCWDelay.h"
#include "buffer_ops.h"
#include "float_math.h"
#include "fx_api.h"
#include "LCWDelayFirParamTable.h"

static inline float softclip(float x)
{
    const float pre = 1.f/4.f;
    const float post = 4.f;

    return fx_softclipf(1.f/3.f, x * pre) * post;
}

static float applyFilter(LCWDelayBuffer *buf, LCWFirParam *fir) {
    const uint32_t mask = buf->mask;
    const uint32_t i = buf->pointer;
    const float *p = buf->buffer;

    const float *param = fir->param;
    const int32_t num = fir->num;
    float ret = 0.f;
    for (int32_t j=0; j<num; j++) {
        ret += (p[(i+j) & mask] * param[j]);
    }

    return ret;
}

static void inputItem(LCWDelayBuffer *buf, float src)
{
    buf->pointer = LCW_DELAY_BUFFER_DEC(buf);
    buf->buffer[buf->pointer] = src;
}

static float convergePosition(float position, float current)
{
    const float param = 0.99979f;
    const float diff = position - current;
    if ( si_fabsf(diff) < 0.001f ) {
        return position;
    }
    else {
        return position - (diff * param);
    }
}

static float lookupBuffer(LCWDelayBuffer *buf, float offset)
{
#if 1
    const int32_t i = (int32_t)offset;
    const float frac = offset - (float)i;
    const int32_t i2 = buf->pointer + i - (LCW_DELAY_FIR_TAP >> 1);
    const int32_t j2 = (int32_t)(frac * LCW_DELAY_FIR_TABLE_SIZE);

    const float *fir = delayFirTable[j2];
    const uint32_t mask = buf->mask;

    float ret = 0;
    for (int32_t k=0; k<LCW_DELAY_FIR_TAP; k++) {
        ret += ( buf->buffer[(i2 + k) & mask] * fir[k] );
    }

    return ret;
#else
    // test
    // const int32_t i = (int32_t)offset;
    // return LCW_DELAY_BUFFER_LUT(buf, i);
    // linear
    const int32_t i = (int32_t)offset;
    const float frac = offset - (float)i;
    const float val1 = LCW_DELAY_BUFFER_LUT(buf, i);
    const float val2 = LCW_DELAY_BUFFER_LUT(buf, i+1);
    return val1 + ((val2 - val1) * frac);
#endif
}

void LCWDelayInit(LCWDelayBlock *block)
{
    buf_clr_f32(block->delayLine->buffer, block->delayLine->size);
    buf_clr_f32(block->inputBuffer->buffer, block->inputBuffer->size);
}

#define LCW_FIR_TAP (11)
static const float firFilterParam[] = {
#if FILTER_TYPE == 1
    -0.00009482, -0.00457797, -0.00717016, 0.06724882, 0.25699051, 0.37426919, 0.25699051, 0.06724882, -0.00717016, -0.00457797, -0.00009482
#elif FILTER_TYPE == 2
    0.02226078, 0.05220225, 0.08696673, 0.11964067, 0.14295034, 0.15140090, 0.14295034, 0.11964067, 0.08696673, 0.05220225, 0.02226078
#else
    #error "FILTER_TYPE is invalid!"
#endif
};

void LCWDelayUpdate(LCWDelayBlock *block)
{
    block->fir.param = &(firFilterParam[0]);
    block->fir.num = LCW_FIR_TAP;
}

float LCWDelayProcess(LCWDelayBlock *block, float input)
{
    // currentPositionをpositionに近づける
    block->currentPosition = convergePosition(block->param.position, block->currentPosition);

    // ディレイの起点を更新
    LCWDelayBuffer *buf = block->delayLine;
    buf->pointer = LCW_DELAY_BUFFER_DEC(buf);

    // フィードバック付きディレイ処理
    const float ret = lookupBuffer(buf, block->currentPosition);
    inputItem(block->inputBuffer, softclip(input + block->param.fbGain * ret));
    buf->buffer[buf->pointer] = applyFilter(block->inputBuffer, &(block->fir));
    return ret;
}
