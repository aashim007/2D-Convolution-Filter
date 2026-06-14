#include "conv2d.h"

void conv2d_baseline(
    data_t input[R][C],
    data_t output[R][C],
    data_t kernel[K][K]
) {
    
    int pad = K / 2;                   

    for (int r = 0; r < R; r++) {
        for (int c = 0; c < C; c++) {
            // double is used because i am taking scipy convolution as reference in which float 64 is converted to float 32 
            // which decreases the tolerance to 1e-4. this double is used to reduce that error and it works for 99% test cases for tolerance 1e-4.
            double sum = 0;                                  

            for (int k = 0; k <K *K; k++) {                   // for nested 3 loops combined 2 loops of kernel
                int i = k / K;
                int j = k % K;

                int rr = r + i - pad;
                int cc = c + j - pad;

                if (rr >= 0 && rr < R && cc >= 0 && cc < C) {
                    sum += (double)input[rr][cc] * (double)kernel[K - 1 - i][K - 1 - j];
                    //sum += input[rr][cc] * kernel[K - 1 - i][K - 1 - j];
                }
            }

            output[r][c] = (data_t)sum;
        }
    }
}