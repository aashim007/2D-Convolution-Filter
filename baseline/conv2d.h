#ifndef CONV_H
#define CONV_H

#include "conv2d_common.h"
#include "conv_kernel.h"

void conv2d_baseline(
    data_t input[R][C],
    data_t output[R][C],
    data_t kernel[K][K]
);

#endif