# Makefile for 2D Convolution HLS Project
#
# Simulation uses the system compiler with Vitis HLS headers.
#
# Required environment:
#   XILINX_HLS    -- path to Vitis HLS installation
#                    (e.g. /opt/Xilinx/Vitis_HLS/2021.1)
#
# Targets:
#   make sim            -- run simulation
#   make sim_baseline   -- compile and run baseline testbench
#   make vectors        -- regenerate test vectors
#   make clean          -- remove build products

# ---- toolchain ----
CXX      := g++
CXXFLAGS := -std=c++14 -g -O0 -Wall -Wno-unknown-pragmas -Wno-int-in-bool-context

XILINX_HLS ?= /mnt/d/Vitis_HLS/2021.1
HLS_INC  := -I$(XILINX_HLS)/include

# ---- file lists ----
COMMON_H   := conv2d_common.h conv_kernel.h

BASELINE_SRC := conv2d_baseline_fixed.cpp
BASELINE_TB  := conv2d_tb.cpp
BASELINE_HDR := conv2d.h $(COMMON_H)

VECTORS := input.txt expected_output.txt conv_kernel.h

# ---- default target ----
all: sim

# ==================================================================
# Test vector generation
# ==================================================================
vectors: $(VECTORS)

$(VECTORS): generate_vectors.py
	@echo "---- Generating test vectors ----"
	python3 generate_vectors_fixed.py

# ==================================================================
# Simulation -- system compiler with Vitis HLS headers
# ==================================================================
sim: sim_baseline

sim_baseline: csim_baseline $(VECTORS)
	@echo "---- Baseline C simulation ----"
	./csim_baseline

csim_baseline: $(BASELINE_SRC) $(BASELINE_TB) $(BASELINE_HDR)
	$(CXX) $(CXXFLAGS) $(HLS_INC) -o $@ $(BASELINE_SRC) $(BASELINE_TB)

# ==================================================================
# Clean
# ==================================================================
clean:
	rm -f conv_kernel.h
	rm -f input.txt expected_output.txt
	rm -f csim_baseline

.PHONY: all sim sim_baseline vectors clean