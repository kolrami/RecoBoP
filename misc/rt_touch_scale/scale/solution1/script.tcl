############################################################
## This file is generated automatically by Vivado HLS.
## Please DO NOT edit it.
## Copyright (C) 2014 Xilinx Inc. All rights reserved.
############################################################
open_project scale
set_top scale
add_files scale.cpp
add_files -tb scale_tb.cpp
open_solution "solution1"
set_part {xc7z020clg484-1}
create_clock -period 10 -name default
source "./scale/solution1/directives.tcl"
csim_design
csynth_design
cosim_design -tool xsim
export_design -evaluate vhdl -format pcore -version "1.00.a" -use_netlist none
