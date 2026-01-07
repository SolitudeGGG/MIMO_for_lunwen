#!/usr/bin/env python3
"""
HLS TCL脚本生成器
为位宽优化生成完整的Vitis HLS TCL脚本
"""

import os

def generate_full_hls_tcl(config_name="bitwidth_opt", 
                          data_base_path=".",
                          project_name="MHGD_xo_out_bitwidth",
                          part="xczu7ev-ffvc1156-2-e",
                          clock_period=10):
    """
    生成完整的HLS TCL脚本，基于用户提供的模板
    
    参数:
        config_name: 配置名称
        data_base_path: 测试数据基础路径
        project_name: HLS项目名称
        part: FPGA器件型号
        clock_period: 时钟周期(ns)
    
    返回:
        TCL脚本文件路径
    """
    
    tcl_content = f"""#
# Bitwidth Optimization HLS TCL Script
# Auto-generated for configuration: {config_name}
#

# Create a project
open_project -reset {project_name}

# Add design files
add_files MHGD_accel_hw.cpp
add_files MHGD_accel_hw.h
add_files MyComplex_1.h

# Add test bench & files
add_files -tb main_hw.cpp

# Add test data files
# Output files (for comparison)
add_files -tb {data_base_path}/output_file/bits_output_SNR=5.txt -cflags "-I."
add_files -tb {data_base_path}/output_file/bits_output_SNR=10.txt -cflags "-I."
add_files -tb {data_base_path}/output_file/bits_output_SNR=15.txt -cflags "-I."
add_files -tb {data_base_path}/output_file/bits_output_SNR=20.txt -cflags "-I."
add_files -tb {data_base_path}/output_file/bits_output_SNR=25.txt -cflags "-I."

# Input H matrices
add_files -tb {data_base_path}/input_file/H_SNR=5.txt -cflags "-I."
add_files -tb {data_base_path}/input_file/H_SNR=10.txt -cflags "-I."
add_files -tb {data_base_path}/input_file/H_SNR=15.txt -cflags "-I."
add_files -tb {data_base_path}/input_file/H_SNR=20.txt -cflags "-I."
add_files -tb {data_base_path}/input_file/H_SNR=25.txt -cflags "-I."

# Input y vectors
add_files -tb {data_base_path}/input_file/y_SNR=5.txt -cflags "-I."
add_files -tb {data_base_path}/input_file/y_SNR=10.txt -cflags "-I."
add_files -tb {data_base_path}/input_file/y_SNR=15.txt -cflags "-I."
add_files -tb {data_base_path}/input_file/y_SNR=20.txt -cflags "-I."
add_files -tb {data_base_path}/input_file/y_SNR=25.txt -cflags "-I."

# Constellation files
add_files -tb {data_base_path}/_64QAM_Constellation.txt -cflags "-I."
add_files -tb {data_base_path}/_16QAM_Constellation.txt -cflags "-I."

# Gaussian noise files
add_files -tb {data_base_path}/gaussian_random_values.txt -cflags "-I."
add_files -tb {data_base_path}/gaussian_random_values_plus.txt -cflags "-I."
add_files -tb {data_base_path}/gaussian_random_values_plus_2.txt -cflags "-I."
add_files -tb {data_base_path}/gaussian_random_values_plus_3.txt -cflags "-I."
add_files -tb {data_base_path}/gaussian_random_values_plus_4.txt -cflags "-I."

# Set the top-level function
set_top MHGD_detect_accel_hw

# Create a solution
open_solution -reset "solution1"
set_part {{{part}}}
create_clock -period {clock_period}

# Configure interface
config_interface -m_axi_max_widen_bitwidth 512
config_interface -m_axi_alignment_byte_size 64

# Run C simulation only (for bitwidth optimization)
csim_design -ldflags "-lm"

# Exit
exit
"""
    
    tcl_file = f"run_hls_{config_name}.tcl"
    with open(tcl_file, 'w', encoding='utf-8') as f:
        f.write(tcl_content)
    
    return tcl_file

def generate_synthesis_tcl(config_name="bitwidth_opt_synth",
                          data_base_path=".",
                          project_name="MHGD_xo_out_bitwidth",
                          part="xczu7ev-ffvc1156-2-e",
                          clock_period=10,
                          hls_exec=1):
    """
    生成包含综合的完整HLS TCL脚本
    
    参数:
        hls_exec: 1=仅Csim, 2=Csim+Csynth, 3=Csim+Csynth+Cosim+Export
    """
    
    tcl_content = f"""#
# Full HLS Flow TCL Script with Synthesis
# Configuration: {config_name}
#

# Create a project
open_project -reset {project_name}

# Add design files
add_files MHGD_accel_hw.cpp
add_files MHGD_accel_hw.h
add_files MyComplex_1.h

# Add test bench & files
add_files -tb main_hw.cpp

# Add test data files
add_files -tb {data_base_path}/output_file/bits_output_SNR=5.txt -cflags "-I."
add_files -tb {data_base_path}/output_file/bits_output_SNR=10.txt -cflags "-I."
add_files -tb {data_base_path}/output_file/bits_output_SNR=15.txt -cflags "-I."
add_files -tb {data_base_path}/output_file/bits_output_SNR=20.txt -cflags "-I."
add_files -tb {data_base_path}/output_file/bits_output_SNR=25.txt -cflags "-I."

add_files -tb {data_base_path}/input_file/H_SNR=5.txt -cflags "-I."
add_files -tb {data_base_path}/input_file/H_SNR=10.txt -cflags "-I."
add_files -tb {data_base_path}/input_file/H_SNR=15.txt -cflags "-I."
add_files -tb {data_base_path}/input_file/H_SNR=20.txt -cflags "-I."
add_files -tb {data_base_path}/input_file/H_SNR=25.txt -cflags "-I."

add_files -tb {data_base_path}/input_file/y_SNR=5.txt -cflags "-I."
add_files -tb {data_base_path}/input_file/y_SNR=10.txt -cflags "-I."
add_files -tb {data_base_path}/input_file/y_SNR=15.txt -cflags "-I."
add_files -tb {data_base_path}/input_file/y_SNR=20.txt -cflags "-I."
add_files -tb {data_base_path}/input_file/y_SNR=25.txt -cflags "-I."

add_files -tb {data_base_path}/_64QAM_Constellation.txt -cflags "-I."
add_files -tb {data_base_path}/_16QAM_Constellation.txt -cflags "-I."
add_files -tb {data_base_path}/gaussian_random_values.txt -cflags "-I."

# Set the top-level function
set_top MHGD_detect_accel_hw

# Create a solution
open_solution -reset "solution1"
set_part {{{part}}}
create_clock -period {clock_period}

# Set variable to select which steps to execute
set hls_exec {hls_exec}

# Configure interface
config_interface -m_axi_max_widen_bitwidth 512
config_interface -m_axi_alignment_byte_size 64

# Run C simulation
csim_design -ldflags "-lm"

# Set any optimization directives (add as needed)
# set_directive_pipeline loop_perfect/LOOP_J

# Execute based on hls_exec value
if {{$hls_exec == 1}} {{
    # Run Synthesis and Exit
    csynth_design
}} elseif {{$hls_exec == 2}} {{
    # Run Synthesis, RTL Simulation and Exit
    csynth_design
    cosim_design
}} elseif {{$hls_exec == 3}} {{ 
    # Run Synthesis, RTL Simulation, RTL implementation and Exit
    csynth_design
    cosim_design
    export_design -format xo -output "./hlsKernel/{config_name}.xo" -rtl verilog
}} else {{
    # Default is Csim only for bitwidth optimization
    # csynth_design
}}

exit
"""
    
    tcl_file = f"run_hls_full_{config_name}.tcl"
    with open(tcl_file, 'w', encoding='utf-8') as f:
        f.write(tcl_content)
    
    return tcl_file

if __name__ == "__main__":
    # 示例用法
    print("生成Csim专用TCL脚本...")
    tcl1 = generate_full_hls_tcl("test_config", data_base_path="/path/to/data")
    print(f"  生成: {tcl1}")
    
    print("\n生成包含综合的完整TCL脚本...")
    tcl2 = generate_synthesis_tcl("test_synth", data_base_path="/path/to/data", hls_exec=1)
    print(f"  生成: {tcl2}")
