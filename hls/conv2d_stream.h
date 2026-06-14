#ifndef CONV2D_STREAM_H
#define CONV2D_STREAM_H

#include <ap_fixed.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>

// ─── Image & Kernel Dimension Limits ─────────────────────────────────────────
#define MAX_COLS  128      // maximum image width  supported
#define MAX_ROWS  128     // maximum image height supported
#define MAX_K     5
// maximum kernel size  (5×5)

// ─── Fixed-Point Types ────────────────────────────────────────────────────────
// Pixel: unsigned 8.0  (0-255 grayscale) — matches data_t role in original
typedef ap_ufixed<8,  8, AP_RND, AP_SAT>   data_t;

// Kernel coefficient: signed 16.8 (fractional weights, e.g. Gaussian)
typedef ap_fixed<16,  8, AP_RND, AP_SAT>   coef_t;

// Accumulator: signed 32.8 — wide enough for 5×5 kernel × 255 × max coef
typedef ap_fixed<32, 24, AP_RND, AP_SAT>   accum_t;

// ─── AXI-Stream Packet (32-bit, matches DMA) ─────────────────────────────────
typedef ap_axis<32, 1, 1, 1>  axis_pkt_t;

// ─── Top-Level Function ───────────────────────────────────────────────────────
void conv2d_linebuff_stream(
    hls::stream<axis_pkt_t> &in_stream,       // input pixel stream
    hls::stream<axis_pkt_t> &out_stream,      // output pixel stream
    coef_t                   kernel[MAX_K][MAX_K],  // AXI-Lite kernel
    int                      img_rows,         // AXI-Lite: actual image height
    int                      img_cols,         // AXI-Lite: actual image width
    int                      k_size            // AXI-Lite: kernel size (3 or 5)
);

#endif /* CONV2D_STREAM_H */
