/*=============================================================================
 *  conv2d_tb2.cpp  â€“  HLS C-Simulation Testbench
 *
 *  Tests
 *  â”€â”€â”€â”€â”€â”€
 *  1. 3Ã—3 identity kernel  â†’ output must equal input on ALL pixels
 *  2. 3Ã—3 box-blur kernel  â†’ compare with golden software reference
 *  3. 5Ã—5 Gaussian kernel  â†’ compare with golden software reference
 *
 *  Pass criterion: ALL pixels (including border) within Â±1 LSB of the
 *  software reference.  The DUT uses true zero-padding so every output
 *  pixel is valid â€” there are no pixels to skip.
 *===========================================================================*/

#include <iostream>
#include <cmath>
#include <cstring>
#include <hls_stream.h>
#include "conv2d_stream.h"

// â”€â”€â”€ Test image size â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
#define IMG_ROWS  16
#define IMG_COLS  16

// â”€â”€â”€ Helpers â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

// Stream a 2-D pixel array into in_stream
void load_image(hls::stream<axis_pkt_t> &s,
                unsigned char img[IMG_ROWS][IMG_COLS],
                int rows, int cols)
{
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            axis_pkt_t pkt;
            pkt.data = img[r][c];
            pkt.keep = 0xF;
            pkt.strb = 0xF;
            pkt.user = 0;
            pkt.id   = 0;
            pkt.dest = 0;
            pkt.last = ((r == rows-1) && (c == cols-1)) ? 1 : 0;
            s.write(pkt);
        }
    }
}

// Drain out_stream into an output array
void drain_image(hls::stream<axis_pkt_t> &m,
                 unsigned char out[IMG_ROWS][IMG_COLS],
                 int rows, int cols)
{
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            axis_pkt_t pkt = m.read();
            out[r][c] = (unsigned char)(pkt.data & 0xFF);
        }
    }
}

// Assert stream is exactly empty after draining.
// Returns 1 if unexpected extra packets remain, 0 if clean.
int assert_stream_empty(hls::stream<axis_pkt_t> &m, const char *label)
{
    if (!m.empty()) {
        std::cout << "[FAIL] " << label
                  << " â€“ out_stream has unexpected extra packets after drain\n";
        while (!m.empty()) m.read();
        return 1;
    }
    return 0;
}

// Software-reference 2-D convolution with TRUE zero-padding (float).
// Out-of-bounds source pixels are treated as 0 â€” matches HLS behaviour exactly.
void sw_convolve(unsigned char in[IMG_ROWS][IMG_COLS],
                 float fkernel[MAX_K][MAX_K],
                 unsigned char out[IMG_ROWS][IMG_COLS],
                 int rows, int cols, int k)
{
    int half = k >> 1;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            float acc = 0.0f;
            for (int kr = 0; kr < k; kr++) {
                for (int kc = 0; kc < k; kc++) {
                    int src_r = r - half + kr;
                    int src_c = c - half + kc;
                    if (src_r >= 0 && src_r < rows &&
                        src_c >= 0 && src_c < cols)
                        acc += in[src_r][src_c] * fkernel[kr][kc];
                }
            }
            if (acc < 0)   acc = 0;
            if (acc > 255) acc = 255;
            out[r][c] = (unsigned char)(acc + 0.5f);
        }
    }
}

// Compare ALL pixels including edges (zero-padding â†’ every pixel valid).
// Returns number of pixels where |HLS - REF| > 1 LSB.
int compare(unsigned char a[IMG_ROWS][IMG_COLS],
            unsigned char b[IMG_ROWS][IMG_COLS],
            int rows, int cols,
            const char *label)
{
    int errors = 0;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int diff = abs((int)a[r][c] - (int)b[r][c]);
            if (diff > 1) {
                std::cout << "[FAIL] " << label
                          << " pixel(" << r << "," << c << ")"
                          << "  HLS=" << (int)a[r][c]
                          << "  REF=" << (int)b[r][c]
                          << "  diff=" << diff << "\n";
                errors++;
            }
        }
    }
    return errors;
}

// â”€â”€â”€ Main â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
int main()
{
    // â”€â”€ Synthetic ramp test image â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    unsigned char img_in[IMG_ROWS][IMG_COLS];
    for (int r = 0; r < IMG_ROWS; r++)
        for (int c = 0; c < IMG_COLS; c++)
            img_in[r][c] = (unsigned char)((r * IMG_COLS + c) & 0xFF);

    // â”€â”€ Kernel storage for HLS DUT â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    coef_t hw_kernel[MAX_K][MAX_K];

    // â”€â”€ Output buffers â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    unsigned char hw_out[IMG_ROWS][IMG_COLS];
    unsigned char sw_out[IMG_ROWS][IMG_COLS];

    int total_errors = 0;

    // =========================================================================
    // Test 1 â€“ 3Ã—3 Identity kernel
    // =========================================================================
    {
        std::cout << "\n=== Test 1: 3x3 Identity Kernel ===\n";
        const int K = 3;

        hls::stream<axis_pkt_t> in_stream("in_stream_t1");
        hls::stream<axis_pkt_t> out_stream("out_stream_t1");

        for (int r = 0; r < MAX_K; r++)
            for (int c = 0; c < MAX_K; c++)
                hw_kernel[r][c] = coef_t(0);
        hw_kernel[1][1] = coef_t(1);

        float fk[MAX_K][MAX_K] = {};
        fk[1][1] = 1.0f;

        load_image(in_stream, img_in, IMG_ROWS, IMG_COLS);
        conv2d_linebuff_stream(in_stream, out_stream, hw_kernel,
                               IMG_ROWS, IMG_COLS, K);
        drain_image(out_stream, hw_out, IMG_ROWS, IMG_COLS);

        sw_convolve(img_in, fk, sw_out, IMG_ROWS, IMG_COLS, K);

        int e = compare(hw_out, sw_out, IMG_ROWS, IMG_COLS, "Identity-3x3");
        e += assert_stream_empty(out_stream, "Identity-3x3");
        total_errors += e;
        if (e == 0) std::cout << "[PASS] Identity 3x3 â€“ all pixels match\n";
    }

    // =========================================================================
    // Test 2 â€“ 3Ã—3 Box-blur (average) kernel  (coef = 1/9 â‰ˆ 0.111)
    // =========================================================================
    {
        std::cout << "\n=== Test 2: 3x3 Box-Blur Kernel ===\n";
        const int K = 3;
        const float inv9 = 1.0f / 9.0f;

        hls::stream<axis_pkt_t> in_stream("in_stream_t2");
        hls::stream<axis_pkt_t> out_stream("out_stream_t2");

        for (int r = 0; r < MAX_K; r++)
            for (int c = 0; c < MAX_K; c++)
                hw_kernel[r][c] = coef_t(0);

        float fk[MAX_K][MAX_K] = {};
        for (int r = 0; r < K; r++)
            for (int c = 0; c < K; c++) {
                coef_t q = coef_t(inv9);
                hw_kernel[r][c] = q;
                fk[r][c] = (float)q;
            }

        load_image(in_stream, img_in, IMG_ROWS, IMG_COLS);
        conv2d_linebuff_stream(in_stream, out_stream, hw_kernel,
                               IMG_ROWS, IMG_COLS, K);
        drain_image(out_stream, hw_out, IMG_ROWS, IMG_COLS);

        sw_convolve(img_in, fk, sw_out, IMG_ROWS, IMG_COLS, K);

        int e = compare(hw_out, sw_out, IMG_ROWS, IMG_COLS, "BoxBlur-3x3");
        e += assert_stream_empty(out_stream, "BoxBlur-3x3");
        total_errors += e;
        if (e == 0) std::cout << "[PASS] Box-blur 3x3 â€“ all pixels match\n";
    }

    // =========================================================================
    // Test 3 â€“ 5Ã—5 Gaussian kernel (Ïƒâ‰ˆ1)
    //   Raw weights (unnormalized):
    //    1  4  6  4  1
    //    4 16 24 16  4
    //    6 24 36 24  6
    //    4 16 24 16  4
    //    1  4  6  4  1   sum = 256
    // =========================================================================
    {
        std::cout << "\n=== Test 3: 5x5 Gaussian Kernel ===\n";
        const int K = 5;
        const float raw[5][5] = {
            { 1, 4, 6, 4, 1},
            { 4,16,24,16, 4},
            { 6,24,36,24, 6},
            { 4,16,24,16, 4},
            { 1, 4, 6, 4, 1}
        };
        const float norm = 1.0f / 256.0f;

        hls::stream<axis_pkt_t> in_stream("in_stream_t3");
        hls::stream<axis_pkt_t> out_stream("out_stream_t3");

        for (int r = 0; r < MAX_K; r++)
            for (int c = 0; c < MAX_K; c++)
                hw_kernel[r][c] = coef_t(0);

        float fk[MAX_K][MAX_K] = {};
        for (int r = 0; r < K; r++)
            for (int c = 0; c < K; c++) {
                coef_t q = coef_t(raw[r][c] * norm);
                hw_kernel[r][c] = q;
                fk[r][c] = (float)q;
            }

        load_image(in_stream, img_in, IMG_ROWS, IMG_COLS);
        conv2d_linebuff_stream(in_stream, out_stream, hw_kernel,
                               IMG_ROWS, IMG_COLS, K);
        drain_image(out_stream, hw_out, IMG_ROWS, IMG_COLS);

        sw_convolve(img_in, fk, sw_out, IMG_ROWS, IMG_COLS, K);

        int e = compare(hw_out, sw_out, IMG_ROWS, IMG_COLS, "Gaussian-5x5");
        e += assert_stream_empty(out_stream, "Gaussian-5x5");
        total_errors += e;
        if (e == 0) std::cout << "[PASS] Gaussian 5x5 â€“ all pixels match\n";
    }

    // â”€â”€â”€ Summary â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    std::cout << "\n===========================================\n";
    if (total_errors == 0)
        std::cout << "ALL TESTS PASSED  âœ“\n";
    else
        std::cout << "FAILED with " << total_errors << " pixel error(s)\n";
    std::cout << "===========================================\n";

    return total_errors ? 1 : 0;
}
