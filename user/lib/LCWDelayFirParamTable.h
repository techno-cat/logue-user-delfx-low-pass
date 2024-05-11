/*
Copyright 2024 Tomoaki Itoh
This software is released under the MIT License, see LICENSE.txt.
//*/

#ifdef __cplusplus
extern "C" {
#endif

#define LCW_DELAY_FIR_TABLE_BITS (8)
#define LCW_DELAY_FIR_TABLE_SIZE (1 << LCW_DELAY_FIR_TABLE_BITS)
#define LCW_DELAY_FIR_TABLE_MASK (0xFF)
#define LCW_DELAY_FIR_TAP (7)

extern const float delayFirTable[][LCW_DELAY_FIR_TAP];

#ifdef __cplusplus
}
#endif
