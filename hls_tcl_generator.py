#!/usr/bin/env python3
"""
HLS TCL脚本生成器
为位宽优化生成完整的Vitis HLS TCL脚本
"""

import os

def generate_full_hls_tcl(config_name="bitwidth_opt", 
                          data_base_path=".",
                          mimo_config_name="8_8_16QAM",
                          gaussian_noise_path="/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai",
                          project_name="MHGD_xo_out_bitwidth",
                          part="xczu7ev-ffvc1156-2-e",
                          clock_period=10):
    """
    生成完整的HLS TCL脚本，基于用户提供的模板
    
    参数:
        config_name: 配置名称
        data_base_path: 测试数据基础路径 (例如: /home/ggg_wufuqi/hls/MHGD/MHGD)
        mimo_config_name: MIMO配置名称 (例如: 8_8_16QAM, 表示8x8天线16QAM调制)
        gaussian_noise_path: 高斯噪声文件路径
        project_name: HLS项目名称
        part: FPGA器件型号
        clock_period: 时钟周期(ns)
    
    返回:
        TCL脚本文件路径
    """
    
    # 构建完整路径
    mimo_data_path = f"{data_base_path}/{mimo_config_name}"
    
    # 获取脚本所在目录，并找到HLS源文件
    # 脚本在PYTHON_COPILOT/目录下，源文件在其父目录
    script_dir = os.path.dirname(os.path.abspath(__file__))
    source_dir = os.path.dirname(script_dir)  # 上一级目录
    
    tcl_content = f"""#
# Bitwidth Optimization HLS TCL Script
# Auto-generated for configuration: {config_name}
# MIMO Config: {mimo_config_name}
#

# Create a project
open_project -reset {project_name}

# Add design files (using absolute paths)
add_files {source_dir}/MHGD_accel_hw.cpp
add_files {source_dir}/MHGD_accel_hw.h
add_files {source_dir}/MyComplex_1.h

# Add test bench & files
add_files -tb {source_dir}/main_hw.cpp

# Add test data files for {mimo_config_name}
# Reference bit files
add_files -tb {mimo_data_path}/reference_file/bits_SNR=5.txt -cflags "-I."
add_files -tb {mimo_data_path}/reference_file/bits_SNR=10.txt -cflags "-I."
add_files -tb {mimo_data_path}/reference_file/bits_SNR=15.txt -cflags "-I."
add_files -tb {mimo_data_path}/reference_file/bits_SNR=20.txt -cflags "-I."
add_files -tb {mimo_data_path}/reference_file/bits_SNR=25.txt -cflags "-I."

# Input H matrices
add_files -tb {mimo_data_path}/input_file/H_SNR=5.txt -cflags "-I."
add_files -tb {mimo_data_path}/input_file/H_SNR=10.txt -cflags "-I."
add_files -tb {mimo_data_path}/input_file/H_SNR=15.txt -cflags "-I."
add_files -tb {mimo_data_path}/input_file/H_SNR=20.txt -cflags "-I."
add_files -tb {mimo_data_path}/input_file/H_SNR=25.txt -cflags "-I."

# Input y vectors
add_files -tb {mimo_data_path}/input_file/y_SNR=5.txt -cflags "-I."
add_files -tb {mimo_data_path}/input_file/y_SNR=10.txt -cflags "-I."
add_files -tb {mimo_data_path}/input_file/y_SNR=15.txt -cflags "-I."
add_files -tb {mimo_data_path}/input_file/y_SNR=20.txt -cflags "-I."
add_files -tb {mimo_data_path}/input_file/y_SNR=25.txt -cflags "-I."

# Gaussian noise files (fixed paths)
add_files -tb {gaussian_noise_path}/gaussian_random_values_plus.txt -cflags "-I."
add_files -tb {gaussian_noise_path}/gaussian_random_values_plus_2.txt -cflags "-I."
add_files -tb {gaussian_noise_path}/gaussian_random_values_plus_3.txt -cflags "-I."
add_files -tb {gaussian_noise_path}/gaussian_random_values_plus_4.txt -cflags "-I."

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
    
    # 保存TCL文件到logfiles目录
    script_dir = os.path.dirname(os.path.abspath(__file__))
    logfiles_dir = os.path.join(script_dir, 'logfiles')
    os.makedirs(logfiles_dir, exist_ok=True)
    tcl_file = os.path.join(logfiles_dir, f"run_hls_{config_name}.tcl")
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
    
    # 保存TCL文件到logfiles目录
    script_dir = os.path.dirname(os.path.abspath(__file__))
    logfiles_dir = os.path.join(script_dir, 'logfiles')
    os.makedirs(logfiles_dir, exist_ok=True)
    tcl_file = os.path.join(logfiles_dir, f"run_hls_full_{config_name}.tcl")
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
