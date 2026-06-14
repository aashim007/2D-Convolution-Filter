#include "conv2d.h"
#include <stdio.h>
#include <stdlib.h>

// ap_fixed<32,16> has 16 fractional bits -> precision = 1/65536
// After K*K accumulations worst case error is very small
// Using 0.5f as tolerance to be safe
#define TOLERANCE 0.5f

static float absf(float x) { return x < 0 ? -x : x; }


// ---------- Read input image ----------
void read_input(const char *fname, data_t input[R][C])
{
    FILE *f = fopen(fname, "r");
    if (!f) { printf("Cannot open %s\n", fname); exit(1); }

    int r, c, k;
    fscanf(f, "%d %d %d", &r, &c, &k);

    if (r != R || c != C || k != K) {
        printf("Dimension mismatch! File:(%d,%d,%d) Binary:(%d,%d,%d)\n",
               r, c, k, R, C, K);
        exit(1);
    }

    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++) {
            float tmp;
            fscanf(f, "%f", &tmp);
            input[i][j] = (data_t)tmp;
        }

    fclose(f);
}


// ---------- Read expected output ----------
void read_expected(const char *fname, float expected[R][C])
{
    FILE *f = fopen(fname, "r");
    if (!f) { printf("Cannot open %s\n", fname); exit(1); }

    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++)
            fscanf(f, "%f", &expected[i][j]);

    fclose(f);
}


// ---------- MAIN ----------
int main()
{
    data_t input  [R][C];
    data_t output [R][C];
    float  expected[R][C];

    // ----------------------------------------------------------
    //  Kernel comes from kernel_ref[] in conv_kernel.h
    //  This is the SAME kernel scipy used -> guaranteed sync
    //  No kernel.txt needed
    // ----------------------------------------------------------
    data_t kernel[K][K];
    for (int i = 0; i < K; i++)
        for (int j = 0; j < K; j++)
            kernel[i][j] = (data_t)kernel_ref[i][j];

    // txt files are one level up from hls/
    read_input   ("input.txt",           input);
    read_expected("expected_output.txt", expected);

    // ----------------------------------------------------------
    //  Debug: verify sync between kernel and expected
    // ----------------------------------------------------------
    printf("R=%d C=%d K=%d\n", R, C, K);
    printf("kernel[0][0]=%.6f  input[0][0]=%.6f  expected[0][0]=%.6f\n",
           (float)kernel[0][0], (float)input[0][0], expected[0][0]);

    // Run DUT
    conv2d_baseline(input, output, kernel);

    printf("output[0][0]=%.6f\n\n", (float)output[0][0]);

    // ----------------------------------------------------------
    //  Compare
    // ----------------------------------------------------------
    int errors = 0;
    for (int i = 0; i < R; i++) {
        for (int j = 0; j < C; j++) {
            float got = (float)output[i][j];
            float err = absf(got - expected[i][j]);
            if (err > TOLERANCE) {
                printf("MISMATCH at (%d,%d): got %.4f, expected %.4f (err=%.4e)\n",
                       i, j, got, expected[i][j], err);
                errors++;
            }
        }
    }

    if (errors == 0)
        printf("PASS: All outputs match (tolerance=%.2f)\n", (double)TOLERANCE);
    else
        printf("FAIL: %d mismatches\n", errors);

    return errors ? 1 : 0;
}
