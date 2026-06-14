#ifndef CONV_COMMON_H
#define CONV_COMMON_H

#define USE_FIXED

#ifdef USE_FIXED
#include <ap_fixed.h>
typedef ap_fixed<32, 16> data_t;
typedef ap_fixed<32,16> accum_t;
#define DATA_WIDTH 32
#else
typedef float data_t;
#define DATA_WIDTH 32
#endif

#endif
