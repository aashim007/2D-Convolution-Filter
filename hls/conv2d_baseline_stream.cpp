

#include "conv2d_stream.h"

void conv2d_baseline_stream(
    hls::stream<axis_pkt_t> &in_stream,
    hls::stream<axis_pkt_t> &out_stream,
    coef_t                   kernel[MAX_K][MAX_K],
    int                      img_rows,
    int                      img_cols,
    int                      k_size)
{
// ── Interface pragmas (identical to linebuff version) ────────────────────────
#pragma HLS INTERFACE axis      port=in_stream
#pragma HLS INTERFACE axis      port=out_stream
#pragma HLS INTERFACE s_axilite port=kernel    bundle=CTRL
#pragma HLS INTERFACE s_axilite port=img_rows  bundle=CTRL
#pragma HLS INTERFACE s_axilite port=img_cols  bundle=CTRL
#pragma HLS INTERFACE s_axilite port=k_size    bundle=CTRL
#pragma HLS INTERFACE s_axilite port=return    bundle=CTRL

// ── Local image buffers ───────────────────────────────────────────────────────
    data_t input [MAX_ROWS][MAX_COLS];
    data_t output[MAX_ROWS][MAX_COLS];

// ── Step 1: Read image from AXI-Stream ───────────────────────────────────────
    READ_R:
    for (int r = 0; r < img_rows; r++) {
        READ_C:
        for (int c = 0; c < img_cols; c++) {
            axis_pkt_t pkt = in_stream.read();
            input[r][c] = data_t(pkt.data & 0xFF);   // low byte = pixel value
        }
    }

// ── Step 2: Naive convolution — no pragmas, purely sequential ────────────────
    int half = k_size / 2;

    CONV_R:
    for (int r = 0; r < img_rows; r++) {
        CONV_C:
        for (int c = 0; c < img_cols; c++) {

            accum_t sum = accum_t(0);

            KERN_R:
            for (int i = 0; i < MAX_K; i++) {
                KERN_C:
                for (int j = 0; j < MAX_K; j++) {

                    // Only use taps within the active k_size x k_size region
                    if (i < k_size && j < k_size) {
                        int rr = r + i - half;
                        int cc = c + j - half;

                        // Zero-padding: skip pixels outside the image boundary
                        if (rr >= 0 && rr < img_rows &&
                            cc >= 0 && cc < img_cols) {
                            sum += accum_t(input[rr][cc]) *
                                   accum_t(kernel[i][j]);
                        }
                    }
                }
            }

            // Clamp to [0, 255]
            if      (sum < accum_t(0))   output[r][c] = data_t(0);
            else if (sum > accum_t(255)) output[r][c] = data_t(255);
            else                          output[r][c] = data_t(sum);
        }
    }

// ── Step 3: Write output to AXI-Stream ───────────────────────────────────────
    int total = img_rows * img_cols;
    int count = 0;

    WRITE_R:
    for (int r = 0; r < img_rows; r++) {
        WRITE_C:
        for (int c = 0; c < img_cols; c++) {
            axis_pkt_t pkt;
            pkt.data = ap_uint<32>(output[r][c]);
            pkt.keep = 0xF;
            pkt.strb = 0xF;
            pkt.user = 0;
            pkt.id   = 0;
            pkt.dest = 0;
            count++;
            pkt.last = (count == total) ? 1 : 0;
            out_stream.write(pkt);
        }
    }
}
