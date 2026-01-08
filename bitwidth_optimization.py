#!/usr/bin/env python3
"""
固定点位宽自动优化脚本
基于BER性能逐步削减变量位宽，找到最优位宽配置
使用Vitis HLS进行Csim测试
"""

import re
import os
import sys
import subprocess
import json
from pathlib import Path
from jinja2 import Template

# 导入HLS TCL生成器
try:
    from hls_tcl_generator import generate_full_hls_tcl
except ImportError:
    # 如果无法导入，提供内联版本
    def generate_full_hls_tcl(config_name="bitwidth_opt", 
                              data_base_path=".",
                              mimo_config_name="8_8_16QAM",
                              gaussian_noise_path="/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai",
                              project_name="MHGD_xo_out_bitwidth",
                              part="xczu7ev-ffvc1156-2-e",
                              clock_period=10):
        """简化的TCL生成器 - 与hls_tcl_generator.py保持兼容的签名"""
        tcl_content = f"""
open_project -reset bitwidth_opt_{config_name}
add_files MHGD_accel_hw.cpp
add_files MHGD_accel_hw.h
add_files MyComplex_1.h
add_files -tb main_hw.cpp
set_top MHGD_detect_accel_hw
open_solution -reset "solution1"
set_part {{{part}}}
create_clock -period {clock_period}
config_interface -m_axi_max_widen_bitwidth 512
config_interface -m_axi_alignment_byte_size 64
csim_design -ldflags "-lm"
exit
"""
        # 保存TCL文件到logfiles目录
        script_dir = os.path.dirname(os.path.abspath(__file__))
        logfiles_dir = os.path.join(script_dir, 'logfiles')
        os.makedirs(logfiles_dir, exist_ok=True)
        tcl_file = os.path.join(logfiles_dir, f"run_hls_{config_name}.tcl")
        with open(tcl_file, 'w') as f:
            f.write(tcl_content)
        return tcl_file

class BitwidthOptimizer:
    def __init__(self, header_file, template_file, reference_ber_threshold=1e-4):
        """
        初始化位宽优化器
        
        参数:
            header_file: MyComplex_1.h头文件路径
            template_file: jinja2模板文件路径
            reference_ber_threshold: BER差异阈值(相对于64位基准)
        """
        self.header_file = header_file
        self.template_file = template_file
        self.reference_ber_threshold = reference_ber_threshold
        self.variables = {}
        self.baseline_ber = None
        
    def parse_header_file(self, initial_W=40, initial_I=8):
        """
        解析头文件，提取所有变量及其位宽信息
        
        参数:
            initial_W: 初始总位宽（默认40，即整数8位+小数32位）
            initial_I: 初始整数位宽（默认8位）
        """
        print(f"解析头文件: {self.header_file}")
        print(f"  初始位宽设置: W={initial_W}, I={initial_I} (小数{initial_W-initial_I}位)")
        
        with open(self.header_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 匹配 typedef ap_fixed<W, I> var_name_t; 格式
        pattern = r'typedef\s+ap_fixed<(\d+),\s*(\d+)(?:,\s*[A-Z_]+)?(?:,\s*[A-Z_]+)?>\s+(\w+)_t;'
        matches = re.findall(pattern, content)
        
        for width, int_bits, var_name in matches:
            # 使用文件中定义的位宽，或使用指定的初始位宽（取较大者以确保从足够大的位宽开始）
            file_W = int(width)
            file_I = int(int_bits)
            
            # 如果文件中的位宽小于指定的初始位宽，使用初始位宽
            use_W = max(file_W, initial_W)
            use_I = file_I  # 整数位保持文件定义
            
            self.variables[var_name] = {
                'init_W': use_W,
                'init_I': use_I,
                'current_W': use_W,
                'current_I': use_I,
                'min_W': use_I + 2,  # 至少保留整数位+符号位+2位小数
                'optimized': False,
                'file_W': file_W,  # 记录文件原始位宽供参考
                'file_I': file_I
            }
        
        print(f"  找到 {len(self.variables)} 个变量")
        for var_name, config in list(self.variables.items())[:5]:
            print(f"    {var_name}: W={config['init_W']}, I={config['init_I']} (文件: W={config['file_W']})")
        if len(self.variables) > 5:
            print(f"    ... 还有 {len(self.variables) - 5} 个变量")
        
        return self.variables
    
    def generate_header_from_template(self, output_file, var_configs):
        """使用jinja2模板生成新的头文件"""
        with open(self.template_file, 'r', encoding='utf-8') as f:
            template_content = f.read()
        
        template = Template(template_content)
        # 支持两种模板格式：使用 'vars' 或 'main_vars'
        rendered = template.render(vars=var_configs, main_vars=var_configs)
        
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(rendered)
    
    def generate_hls_tcl_script(self, config_name, mimo_config, data_base_path=".", 
                               gaussian_noise_path="/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai"):
        """
        生成HLS TCL脚本用于Csim
        
        参数:
            config_name: 配置名称
            mimo_config: MIMO配置字典 (包含Nt, Nr, modulation, SNR)
            data_base_path: 测试数据基础路径
            gaussian_noise_path: 高斯噪声文件路径
        """
        # 构建MIMO配置名称，例如: 8_8_16QAM
        nt = mimo_config.get('Nt', 8)
        nr = mimo_config.get('Nr', 8)
        modulation = mimo_config.get('modulation', 16)
        mimo_config_name = f"{nt}_{nr}_{modulation}QAM"
        
        # 使用更完整的TCL生成器
        tcl_file = generate_full_hls_tcl(
            config_name=config_name,
            data_base_path=data_base_path,
            mimo_config_name=mimo_config_name,
            gaussian_noise_path=gaussian_noise_path
        )
        
        return tcl_file
    
    def compile_and_test(self, config_name, var_configs, mimo_config, data_base_path=".", 
                        gaussian_noise_path="/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai"):
        """
        使用Vitis HLS编译并运行Csim测试
        
        参数:
            config_name: 配置名称
            var_configs: 变量位宽配置
            mimo_config: MIMO测试配置 (Nt, Nr, modulation, SNR)
            data_base_path: 测试数据文件的基础路径
            gaussian_noise_path: 高斯噪声文件路径
        
        返回:
            BER值，如果编译或测试失败返回None
        """
        print(f"  测试配置: {config_name}")
        
        # 生成临时头文件
        temp_header = "MyComplex_1_temp.h"
        self.generate_header_from_template(temp_header, var_configs)
        
        # 备份原始头文件
        if os.path.exists("MyComplex_1.h"):
            os.rename("MyComplex_1.h", "MyComplex_1.h.bak")
        os.rename(temp_header, "MyComplex_1.h")
        
        # 生成HLS TCL脚本
        tcl_file = self.generate_hls_tcl_script(config_name, mimo_config, data_base_path, gaussian_noise_path)
        
        try:
            # 运行Vitis HLS
            print(f"    运行Vitis HLS Csim...")
            hls_cmd = ["vitis_hls", "-f", tcl_file]
            
            # 保存日志文件到logfiles目录
            script_dir = os.path.dirname(os.path.abspath(__file__))
            logfiles_dir = os.path.join(script_dir, 'logfiles')
            os.makedirs(logfiles_dir, exist_ok=True)
            log_file = os.path.join(logfiles_dir, f"build_{config_name}.log")
            
            with open(log_file, 'w') as log:
                result = subprocess.run(
                    hls_cmd, 
                    stdout=log, 
                    stderr=subprocess.STDOUT, 
                    text=True, 
                    timeout=600  # 10分钟超时
                )
            
            # 读取日志文件查找BER
            with open(log_file, 'r') as log:
                log_content = log.read()
            
            # 从日志中提取BER
            # 首先尝试查找FINAL_BER标记
            ber_match = re.search(r'FINAL_BER:\s*([\d.e+-]+)', log_content)
            
            if ber_match:
                ber = float(ber_match.group(1))
                print(f"    BER = {ber:.8f}")
                return ber
            else:
                # 尝试其他可能的BER输出格式
                ber_match = re.search(r'BER\s*[=:]\s*([\d.e+-]+)', log_content, re.IGNORECASE)
                if ber_match:
                    ber = float(ber_match.group(1))
                    print(f"    BER = {ber:.8f}")
                    return ber
                
                print(f"    无法从日志中提取BER")
                print(f"    日志文件: {log_file}")
                return None
                
        except subprocess.TimeoutExpired:
            print(f"    HLS执行超时")
            return None
        except FileNotFoundError:
            print(f"    错误: 找不到vitis_hls命令")
            print(f"    请确保Vitis HLS已安装并在PATH中")
            return None
        except Exception as e:
            print(f"    发生错误: {e}")
            return None
        finally:
            # 恢复原始头文件
            if os.path.exists("MyComplex_1.h"):
                os.rename("MyComplex_1.h", temp_header)
            if os.path.exists("MyComplex_1.h.bak"):
                os.rename("MyComplex_1.h.bak", "MyComplex_1.h")
            
            # 清理临时文件
            if os.path.exists(temp_header):
                os.remove(temp_header)
            
            # 日志和TCL文件已保存在logfiles目录中，不需要清理
            # HLS项目文件夹会被HLS创建，可在需要时手动清理
    
    def get_baseline_ber(self, mimo_config):
        """获取64位全精度基准BER"""
        print("获取64位全精度基准BER...")
        
        # 使用当前(全精度)配置
        baseline_config = {var: self.variables[var] for var in self.variables}
        
        data_path = mimo_config.get('data_path', '.')
        gaussian_noise_path = mimo_config.get('gaussian_noise_path', '/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai')
        ber = self.compile_and_test("baseline", baseline_config, mimo_config, data_path, gaussian_noise_path)
        
        if ber is None:
            raise RuntimeError("无法获取基准BER")
        
        self.baseline_ber = ber
        print(f"基准BER: {ber:.8f}")
        return ber
    
    def optimize_single_variable(self, var_name, mimo_config, step=1):
        """
        优化单个变量的位宽
        
        参数:
            var_name: 变量名
            mimo_config: MIMO配置
            step: 每次减少的位宽步长
        
        返回:
            优化后的位宽配置
        """
        print(f"\n优化变量: {var_name}")
        var_config = self.variables[var_name]
        
        current_W = var_config['current_W']
        current_I = var_config['current_I']
        min_W = var_config['min_W']
        
        best_W = current_W
        best_I = current_I
        
        data_path = mimo_config.get('data_path', '.')
        gaussian_noise_path = mimo_config.get('gaussian_noise_path', '/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai')
        
        # 从当前位宽开始，逐步减小
        for test_W in range(current_W, min_W - 1, -step):
            # 构建测试配置
            test_config = {var: self.variables[var].copy() for var in self.variables}
            test_config[var_name]['current_W'] = test_W
            
            # 测试此配置
            ber = self.compile_and_test(f"{var_name}_W{test_W}", test_config, mimo_config, data_path, gaussian_noise_path)
            
            if ber is None:
                # 编译或运行失败，停止优化此变量
                print(f"  W={test_W} 失败，使用W={best_W}")
                break
            
            # 检查BER是否在可接受范围内
            ber_diff = abs(ber - self.baseline_ber)
            
            if ber_diff <= self.reference_ber_threshold or ber <= self.baseline_ber * 1.1:
                # BER可接受，继续减小位宽
                best_W = test_W
                print(f"  W={test_W} 通过 (BER={ber:.8f}, diff={ber_diff:.8f})")
            else:
                # BER超出阈值，停止
                print(f"  W={test_W} BER过高 (BER={ber:.8f}, diff={ber_diff:.8f})")
                break
        
        # 更新变量配置
        self.variables[var_name]['current_W'] = best_W
        self.variables[var_name]['optimized'] = True
        
        print(f"  最优位宽: W={best_W}, I={best_I}")
        
        return best_W, best_I
    
    def optimize_all_variables(self, mimo_config, optimization_order='sequential'):
        """
        优化所有变量的位宽
        
        参数:
            mimo_config: MIMO配置
            optimization_order: 优化顺序 ('sequential', 'high_to_low', 'low_to_high')
        """
        print("\n" + "=" * 60)
        print("开始位宽优化")
        print("=" * 60)
        
        # 获取基准BER
        self.get_baseline_ber(mimo_config)
        
        # 确定优化顺序
        if optimization_order == 'high_to_low':
            # 按初始位宽从大到小排序
            var_list = sorted(self.variables.keys(), 
                            key=lambda v: self.variables[v]['init_W'], 
                            reverse=True)
        elif optimization_order == 'low_to_high':
            # 按初始位宽从小到大排序
            var_list = sorted(self.variables.keys(), 
                            key=lambda v: self.variables[v]['init_W'])
        else:
            # 顺序优化
            var_list = list(self.variables.keys())
        
        print(f"\n优化顺序: {optimization_order}")
        print(f"变量数量: {len(var_list)}")
        
        # 逐个优化变量
        for idx, var_name in enumerate(var_list, 1):
            print(f"\n[{idx}/{len(var_list)}]", end=" ")
            self.optimize_single_variable(var_name, mimo_config, step=2)
        
        print("\n" + "=" * 60)
        print("位宽优化完成")
        print("=" * 60)
    
    def save_optimized_config(self, output_file, mimo_config):
        """保存优化后的配置到JSON文件"""
        config = {
            'mimo_config': mimo_config,
            'baseline_ber': self.baseline_ber,
            'reference_ber_threshold': self.reference_ber_threshold,
            'variables': self.variables
        }
        
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(config, f, indent=2, ensure_ascii=False)
        
        print(f"\n配置已保存到: {output_file}")
    
    def generate_optimized_header(self, output_file):
        """生成优化后的头文件"""
        # 准备优化后的变量配置
        optimized_vars = {}
        for var_name, config in self.variables.items():
            optimized_vars[var_name] = {
                'init_W': config['current_W'],
                'init_I': config['current_I']
            }
        
        # 使用模板生成
        self.generate_header_from_template(output_file, optimized_vars)
        
        print(f"优化后的头文件已生成: {output_file}")
        
        # 打印优化摘要
        print("\n优化摘要:")
        print(f"{'变量名':<30} {'原始位宽':<15} {'优化位宽':<15} {'减少':<10}")
        print("-" * 70)
        
        total_reduction = 0
        for var_name in sorted(self.variables.keys()):
            config = self.variables[var_name]
            init_W = config['init_W']
            curr_W = config['current_W']
            reduction = init_W - curr_W
            total_reduction += reduction
            
            print(f"{var_name:<30} {init_W:<15} {curr_W:<15} {reduction:<10}")
        
        print("-" * 70)
        print(f"总位宽减少: {total_reduction} bits")

def main():
    """主函数"""
    import argparse
    
    # 获取脚本所在目录（PYTHON_COPILOT/）
    script_dir = os.path.dirname(os.path.abspath(__file__))
    source_dir = os.path.dirname(script_dir)  # 上一级目录（mimo_cpp_gai/）
    
    # 创建logfiles目录用于存放所有日志和TCL文件
    logfiles_dir = os.path.join(script_dir, 'logfiles')
    os.makedirs(logfiles_dir, exist_ok=True)
    
    parser = argparse.ArgumentParser(description='MIMO HLS位宽自动优化工具')
    parser.add_argument('--header', 
                       default=os.path.join(source_dir, 'MyComplex_1.h'), 
                       help='输入头文件路径 (默认: ../MyComplex_1.h)')
    parser.add_argument('--template', 
                       default=os.path.join(script_dir, 'sensitivity_types.hpp.jinja2'),
                       help='Jinja2模板文件路径 (默认: ./sensitivity_types.hpp.jinja2)')
    parser.add_argument('--ber-threshold', type=float, default=1e-4,
                       help='BER差异阈值')
    parser.add_argument('--nt', type=int, default=8,
                       help='发送天线数')
    parser.add_argument('--nr', type=int, default=8,
                       help='接收天线数')
    parser.add_argument('--modulation', type=int, choices=[4, 16, 64], default=16,
                       help='调制阶数')
    parser.add_argument('--snr', type=int, default=20,
                       help='信噪比(dB)')
    parser.add_argument('--output-config', default=None,
                       help='输出配置JSON文件路径')
    parser.add_argument('--output-header', default=None,
                       help='输出优化后的头文件路径')
    parser.add_argument('--order', choices=['sequential', 'high_to_low', 'low_to_high'],
                       default='high_to_low',
                       help='优化顺序')
    parser.add_argument('--data-path', default='/home/ggg_wufuqi/hls/MHGD/MHGD',
                       help='HLS测试数据文件的基础路径 (例如: /home/ggg_wufuqi/hls/MHGD/MHGD)')
    parser.add_argument('--gaussian-noise-path', 
                       default='/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai',
                       help='高斯噪声文件路径')
    parser.add_argument('--initial-W', type=int, default=40,
                       help='初始总位宽 (默认: 40, 即整数8位+小数32位)')
    parser.add_argument('--initial-I', type=int, default=8,
                       help='初始整数位宽 (默认: 8)')
    
    args = parser.parse_args()
    
    # 构建MIMO配置
    mimo_config = {
        'Nt': args.nt,
        'Nr': args.nr,
        'modulation': args.modulation,
        'SNR': args.snr,
        'data_path': args.data_path,
        'gaussian_noise_path': args.gaussian_noise_path
    }
    
    # 默认输出文件名 - 保存到PYTHON_COPILOT/bitwidth_result文件夹
    if args.output_config is None:
        # 创建bitwidth_result文件夹在脚本所在目录
        output_dir = os.path.join(script_dir, 'bitwidth_result')
        os.makedirs(output_dir, exist_ok=True)
        args.output_config = os.path.join(output_dir, f"bitwidth_config_{args.nt}x{args.nr}_{args.modulation}QAM_SNR{args.snr}.json")
    
    if args.output_header is None:
        # 创建bitwidth_result文件夹（如果还没创建）
        output_dir = os.path.join(script_dir, 'bitwidth_result')
        os.makedirs(output_dir, exist_ok=True)
        args.output_header = os.path.join(output_dir, f"MyComplex_optimized_{args.nt}x{args.nr}_{args.modulation}QAM_SNR{args.snr}.h")
    
    # 创建优化器
    optimizer = BitwidthOptimizer(args.header, args.template, args.ber_threshold)
    
    # 解析头文件，使用指定的初始位宽
    optimizer.parse_header_file(initial_W=args.initial_W, initial_I=args.initial_I)
    
    # 执行优化
    optimizer.optimize_all_variables(mimo_config, args.order)
    
    # 保存结果
    optimizer.save_optimized_config(args.output_config, mimo_config)
    optimizer.generate_optimized_header(args.output_header)
    
    print("\n优化流程完成！")

if __name__ == "__main__":
    main()
