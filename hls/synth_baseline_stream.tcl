# synth_baseline_stream.tcl -- HLS synthesis + IP export
# for 2D convolution baseline (AXI-Stream version)
#
# Usage:  vitis_hls -f synth_baseline_stream.tcl
#         (or: make synth_baseline_stream)

set PART     "xc7z020clg400-1"
set CLOCK_NS "10"
set TOP      "conv2d_baseline_stream"
set PROJ     "hls_conv2d_baseline_stream"

open_project ${PROJ} -reset
set_top ${TOP}
add_files conv2d_baseline_stream.cpp
add_files conv2d_stream.h
add_files conv2d_common.h
add_files conv_kernel.h

open_solution "sol1" -flow_target vivado -reset
set_part ${PART}
create_clock -period ${CLOCK_NS} -name default

csynth_design

export_design -format ip_catalog \
    -description "2D Convolution Baseline AXI-Stream (${TOP})" \
    -vendor "ee5332" -library "hls" -version "1.0"

exit