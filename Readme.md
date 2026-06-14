#  2D Convolution


## Overview

This file contains two folders one with axi stream implemented in them (hls folder) and other containing baseline code (both float and fixed as explained below in directory structure).

## Important
Main files are 
1 conv2d_linebuff_stream.cpp   // path = hls/conv2d_linebuff_stream.cpp
2 conv2d_stream.h              // path = hls/conv2d_stream.h
3 build_bitstream.tcl          // this folder only
4 conv2d_pynq.ipynb            // this folder only


## Directory Structure

```
hls/                        
  conv2d_stream.h
  conv2d_linebuff_stream.cpp
  conv2d_baseline_stream.cpp
  conv2d_tb2.cpp
  synth_linebuff_stream.tcl        // for hls synthesis directly
  synth_baseline_stream.tcl        // for hls synthesis directly

baseline/                         //  only for reference not main code
  conv2d.h 
  conv2d_common.h      
  conv2d_baseline.cpp
  conv2d_baseline_fixed.cpp        // fixed point implemented
  conv2d_tb.cpp
  generate_vectors.py
  generate_vectors_fixed.py
  Makefile


build_bitstream.tcl                // block design in vivado
design_1_wrapper.bit
design_1.hwh
conv2d_pynq.ipynb
```

## Build Flow

### Step 1: If you want to check baseline first, then generate test vectors

You can check both baseline float and baseline fixed by making a small change as follows
1. conv2d_common.h : Line 4 USE_FIXED or USE_FLOAT
2. Makefile: Line 33 conv2d_baseline_fixed.cpp for fixed point and conv2d_baseline.cpp for float
3. Makefile: Line 49 generate_vectors_fixed.py for fixed point and generate_vectors.py for float

```bash
cd baseline
make sim          # generates vectors, compiles with system g++, run the testbench
```

### Step 2: HLS synthesis and IP export

```bash
cd ../hls          // here you will find all the files needed for synthesis
```

synth_baseline_stream.tcl and synth_linebuff_stream.tcl are attached in hls directory. You can use this by sourcing vitis and running these tcl files. 

Note: Only linebuff_stream ip will be used in vivado block design tcl script

You can run directly the cpp and header file in vitis for generating ip


### Step 3: Vivado block design and bitstream
build_bitstream.tcl is attached in main folder. 

Run it directly on vivado or source it and run it.

### Important
Double check the paths before running this tcl script. Here paths may be different

This creates the block design, runs synthesis and implementation,
and copies the bitstream and .hwh to the main folder

### Note: 
I have changed my axi_dma width of buffer length to 26 after running this tcl file. 

I had made these changes in tcl file You can use as it is just wanted to inform that in my current folder the name of bitfile and hardware files is as 'design_1.hwh' and design_1_wrapper.bit' but now when u run this tcl script then names of files will be 'conv2d.bit' and 'con2d.hwh'



### Step 4: Run on Pynq

Copy `conv2d.bit`, `conv2d.hwh`, and
`conv2d_pynq.ipynb` to the Pynq board and run the notebook.

## Important:
My .ipynb file has bitfile name as 'design_1_wrapper.bit'. Change it to 'conv2d.bit' if you are running it using newly generated bitfile from vivado tcl script. 
Double check the file names once.






