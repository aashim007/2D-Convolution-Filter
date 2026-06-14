#include "conv2d_stream.h"

//  Internal helper 
static data_t clamp_output(accum_t v) {
#pragma HLS INLINE
    if (v < accum_t(0))   return data_t(0);
    if (v > accum_t(255)) return data_t(255);
    return data_t(v);
}

//  Top-Level 
void conv2d_linebuff_stream(
    hls::stream<axis_pkt_t> &in_stream,
    hls::stream<axis_pkt_t> &out_stream,
    coef_t                   kernel[MAX_K][MAX_K],
    int                      img_rows,
    int                      img_cols,
    int                      k_size)
{
//  Interface pragmas 
#pragma HLS INTERFACE axis      port=in_stream
#pragma HLS INTERFACE axis      port=out_stream
#pragma HLS INTERFACE s_axilite port=kernel    bundle=CTRL
#pragma HLS INTERFACE s_axilite port=img_rows  bundle=CTRL
#pragma HLS INTERFACE s_axilite port=img_cols  bundle=CTRL
#pragma HLS INTERFACE s_axilite port=k_size    bundle=CTRL
#pragma HLS INTERFACE s_axilite port=return    bundle=CTRL

//  Storage 
    // Line buffer: (MAX_K-1) rows x (MAX_COLS+MAX_K) columns.
    // Width MAX_COLS+MAX_K safely covers flush-phase column indices.
    // dim=1 partitioned completely so all MAX_K-1 rows are readable in one
    // cycle mandatory for II=1 on the inner pixel loop.
    data_t line_buf[MAX_K - 1][MAX_COLS + MAX_K];
#pragma HLS ARRAY_PARTITION variable=line_buf dim=1 complete
#pragma HLS RESOURCE          variable=line_buf core=RAM_2P_BRAM

    // Sliding window: MAX_K x MAX_K,
    data_t win[MAX_K][MAX_K];
#pragma HLS ARRAY_PARTITION variable=win dim=0 complete

    
#pragma HLS ARRAY_PARTITION variable=kernel dim=0 complete

//  Initialise to zero 
    INIT_LB_ROW:
    for (int kr = 0; kr < MAX_K - 1; kr++) {
#pragma HLS UNROLL
        INIT_LB_COL:
        for (int kc = 0; kc < MAX_COLS + MAX_K; kc++) {
            line_buf[kr][kc] = data_t(0);
        }
    }

    INIT_WIN_R:
    for (int kr = 0; kr < MAX_K; kr++) {
#pragma HLS UNROLL
        INIT_WIN_C:
        for (int kc = 0; kc < MAX_K; kc++) {
#pragma HLS UNROLL
            win[kr][kc] = data_t(0);
        }
    }

//  Loop bounds 
    
    int half = k_size >> 1;

    
    int ext_rows = img_rows + half;
    int ext_cols = img_cols + half;

    // Pixel counter for tlast generation.
    int out_pixel_count  = 0;
    int total_out_pixels = img_rows * img_cols;

//  Pixel-processing loop 
    ROW_LOOP:
    for (int row = 0; row < ext_rows; row++) {
        COL_LOOP:
        for (int col = 0; col < ext_cols; col++) {
#pragma HLS PIPELINE II=1

            //  1. Read pixel from stream or inject zero for flush 
            data_t new_px;
            bool reading_real = (row < img_rows) && (col < img_cols);
            if (reading_real) {
                axis_pkt_t in_pkt = in_stream.read();
                new_px = data_t(in_pkt.data & 0xFF);   // low byte = pixel value
            } else {
                new_px = data_t(0);                    // virtual zero for flush
            }

            //  2. Shift window LEFT 
            //       Column 0 is discarded; new_px will enter at MAX_K-1.
            //       After shift: win[i][j] = old win[i][j+1]
            SHIFT_WIN_R:
            for (int i = 0; i < MAX_K; i++) {
#pragma HLS UNROLL
                SHIFT_WIN_C:
                for (int j = 0; j < MAX_K - 1; j++) {
#pragma HLS UNROLL
                    win[i][j] = win[i][j + 1];
                }
            }

            //  3. Fill rightmost window column (index MAX_K-1) 
            //       Rows 0..MAX_K-2: from line buffer (older rows, top of image).
            //       Row  MAX_K-1   : new_px (current/newest row).
            FILL_WIN_COL:
            for (int i = 0; i < MAX_K - 1; i++) {
#pragma HLS UNROLL
                win[i][MAX_K - 1] = line_buf[i][col];
            }
            win[MAX_K - 1][MAX_K - 1] = new_px;

            //  4. Update line buffer 
            //       Shift rows up (row 0 = oldest, row MAX_K-2 = most recent).
            //       Row 0 is discarded; new_px stored at bottom (MAX_K-2).
            UPDATE_LB:
            for (int i = 0; i < MAX_K - 2; i++) {
#pragma HLS UNROLL
                line_buf[i][col] = line_buf[i + 1][col];
            }
            line_buf[MAX_K - 2][col] = new_px;

            //  5. Compute MAC with zero-padding 
            int out_r = row - half;
            int out_c = col - half;
            bool valid_out = (out_r >= 0) && (out_r < img_rows) &&
                             (out_c >= 0) && (out_c < img_cols);

            if (valid_out) {
                int offset = MAX_K - k_size;
                accum_t sum = accum_t(0);

                MAC_R:
                for (int i = 0; i < MAX_K; i++) {
#pragma HLS UNROLL
                    MAC_C:
                    for (int j = 0; j < MAX_K; j++) {
#pragma HLS UNROLL
                        int ki = i - offset;
                        int kj = j - offset;
                        if (ki >= 0 && ki < k_size &&
                            kj >= 0 && kj < k_size) {
                            sum += accum_t(win[i][j]) *
                                   accum_t(kernel[ki][kj]);
                        }
                    }
                }

                //  6. Write output pixel to AXI-Stream 
                axis_pkt_t out_pkt;
                out_pkt.data = ap_uint<32>(clamp_output(sum));
                out_pkt.keep = 0xF;
                out_pkt.strb = 0xF;
                out_pkt.user = 0;
                out_pkt.id   = 0;
                out_pkt.dest = 0;
                out_pixel_count++;
                out_pkt.last = (out_pixel_count == total_out_pixels) ? 1 : 0;
                out_stream.write(out_pkt);
            }
        }
    }
}
