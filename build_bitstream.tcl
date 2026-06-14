set script_dir [file dirname [file normalize [info script]]]
set proj_dir   [file join $script_dir conv2d_proj]
set ip_repo    "C:/Users/aashi/Downloads/project_dsp/hls/hls_conv2d_linebuff_stream/sol1/impl/ip"
set bd_name    design_1
set part_name  xc7z020clg400-1
set board_name tul.com.tw:pynq-z2:part0:1.0

puts "============================================================"
puts "Running Vivado block design build for Project 3: 2D Convolution"
puts "Project directory : $script_dir"
puts "Vivado project    : $proj_dir"
puts "============================================================"

create_project conv2d_proj $proj_dir -part $part_name -force
set_property board_part $board_name [current_project]
set_property ip_repo_paths $ip_repo [current_project]
update_ip_catalog

create_bd_design $bd_name

create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 processing_system7_0
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 \
    -config {make_external "FIXED_IO, DDR" apply_board_preset "1" Master "Disable" Slave "Disable"} \
    [get_bd_cells processing_system7_0]

set_property -dict [list \
    CONFIG.PCW_USE_M_AXI_GP0 {1} \
    CONFIG.PCW_USE_S_AXI_HP0 {1} \
    CONFIG.PCW_USE_S_AXI_HP2 {1} \
    CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {100}] [get_bd_cells processing_system7_0]

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dma:7.1 axi_dma_0
set_property -dict [list \
    CONFIG.c_include_sg {0} \
    CONFIG.c_sg_include_stscntrl_strm {0} \
    CONFIG.c_m_axis_mm2s_tdata_width {32} \
    CONFIG.c_s_axis_s2mm_tdata_width {32} [get_bd_cells axi_dma_0] \
    CONFIG.c_sg_length_width {26}] [get_bd_cells axi_dma_0]

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_timer:2.0 axi_timer_0
create_bd_cell -type ip -vlnv ee5332:hls:conv2d_linebuff_stream:1.0 conv2d_linebuff_stream_0

apply_bd_automation -rule xilinx.com:bd_rule:axi4 \
    -config {Master "/processing_system7_0/M_AXI_GP0" Clk "Auto"} \
    [get_bd_intf_pins conv2d_linebuff_stream_0/s_axi_CTRL]

apply_bd_automation -rule xilinx.com:bd_rule:axi4 \
    -config {Master "/processing_system7_0/M_AXI_GP0" Clk "Auto"} \
    [get_bd_intf_pins axi_dma_0/S_AXI_LITE]

apply_bd_automation -rule xilinx.com:bd_rule:axi4 \
    -config {Master "/processing_system7_0/M_AXI_GP0" Clk "Auto"} \
    [get_bd_intf_pins axi_timer_0/S_AXI]

apply_bd_automation -rule xilinx.com:bd_rule:axi4 \
    -config {Master "/axi_dma_0/M_AXI_MM2S" Clk "Auto"} \
    [get_bd_intf_pins processing_system7_0/S_AXI_HP0]

apply_bd_automation -rule xilinx.com:bd_rule:axi4 \
    -config {Master "/axi_dma_0/M_AXI_S2MM" Clk "Auto"} \
    [get_bd_intf_pins processing_system7_0/S_AXI_HP2]

connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXIS_MM2S] [get_bd_intf_pins conv2d_linebuff_stream_0/in_stream]
connect_bd_intf_net [get_bd_intf_pins conv2d_linebuff_stream_0/out_stream] [get_bd_intf_pins axi_dma_0/S_AXIS_S2MM]

validate_bd_design
save_bd_design

make_wrapper -files [get_files [file join $proj_dir conv2d_proj.srcs sources_1 bd $bd_name $bd_name.bd]] -top
add_files -norecurse [file join $proj_dir conv2d_proj.gen sources_1 bd $bd_name hdl ${bd_name}_wrapper.v]
set_property top ${bd_name}_wrapper [current_fileset]
update_compile_order -fileset sources_1

launch_runs impl_1 -to_step write_bitstream -jobs 4
wait_on_run impl_1

set bit_src [file join $proj_dir conv2d_proj.runs impl_1 ${bd_name}_wrapper.bit]
set hwh_src [file join $proj_dir conv2d_proj.gen sources_1 bd $bd_name hw_handoff ${bd_name}.hwh]
set hwh_src_alt [file join $proj_dir conv2d_proj.srcs sources_1 bd $bd_name hw_handoff ${bd_name}.hwh]

if {[file exists $bit_src]} {
    file copy -force $bit_src [file join $script_dir conv2d.bit]
}

if {[file exists $hwh_src]} {
    file copy -force $hwh_src [file join $script_dir conv2d.hwh]
} elseif {[file exists $hwh_src_alt]} {
    file copy -force $hwh_src_alt [file join $script_dir conv2d.hwh]
}

write_hw_platform -fixed -include_bit -force -file [file join $script_dir design_1.xsa]

puts "============================================================"
puts "Vivado build complete."
puts "Bitstream copied to [file join $script_dir conv2d.bit]"
puts "Hardware handoff copied to [file join $script_dir conv2d.hwh]"
puts "XSA written to [file join $script_dir design_1.xsa]"
puts "============================================================"