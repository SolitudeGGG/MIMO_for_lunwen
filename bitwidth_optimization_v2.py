#!/usr/bin/env python3
"""
完全优化的固定点位宽自动优化脚本 V2
- 只优化MyComplex_1.h中的45个typedef变量
- 智能搜索策略：小数位先4-bit跳跃，遇到问题逐bit精细搜索
- 整数位每次减少2-bit，下限4-bit
- 最终结果增加2位小数以增强鲁棒性
"""

import re
import os
import sys
import subprocess
import json
import time
import math
import glob
from pathlib import Path
from jinja2 import Template

# 导入HLS TCL生成器
try:
    from hls_tcl_generator import generate_full_hls_tcl
except ImportError:
    def generate_full_hls_tcl(config_name="bitwidth_opt", 
                              data_base_path=".",
                              mimo_config_name="8_8_16QAM",
                              gaussian_noise_path="/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai",
                              project_name="MHGD_xo_out_bitwidth",
                              part="xczu7ev-ffvc1156-2-e",
                              clock_period=10):
        """简化的TCL生成器"""
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
        script_dir = os.path.dirname(os.path.abspath(__file__))
        logfiles_dir = os.path.join(script_dir, 'logfiles')
        os.makedirs(logfiles_dir, exist_ok=True)
        tcl_file = os.path.join(logfiles_dir, f"run_hls_{config_name}.tcl")
        with open(tcl_file, 'w') as f:
            f.write(tcl_content)
        return tcl_file


class OptimizedBitwidthOptimizer:
    """完全优化的位宽优化器"""
    
    def __init__(self, header_file, template_file, output_dir, ber_threshold=0.01):
        """
        初始化优化器
        
        参数:
            header_file: MyComplex_1.h路径
            template_file: jinja2模板路径
            output_dir: 输出目录
            ber_threshold: BER阈值（绝对值，默认0.01）
        """
        self.header_file = header_file
        self.template_file = template_file
        self.output_dir = output_dir
        self.ber_threshold = ber_threshold
        self.variables = {}
        self.baseline_ber = None
        self.optimization_results = {}
        
        # 创建输出目录
        os.makedirs(self.output_dir, exist_ok=True)
        os.makedirs(os.path.join(os.path.dirname(__file__), 'logfiles'), exist_ok=True)
    
    def parse_header_typedef_only(self):
        """
        只解析头文件中的typedef变量（不包含子函数局部变量）
        返回变量字典
        """
        print(f"\n解析头文件: {self.header_file}")
        print(f"  初始位宽: W=40, I=8")
        
        with open(self.header_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 匹配 typedef ap_fixed<W, I> var_name_t;
        pattern = r'typedef\s+ap_fixed<(\d+),\s*(\d+)(?:,\s*[A-Z_]+)?(?:,\s*[A-Z_]+)?>\s+(\w+)_t\s*;'
        matches = re.findall(pattern, content)
        
        for width, int_bits, var_name in matches:
            # 忽略以_subfunc_开头的子函数变量
            if var_name.startswith('_subfunc_'):
                continue
            
            self.variables[var_name] = {
                'init_W': 40,  # 统一从40开始
                'init_I': 8,   # 统一从8开始
                'current_W': 40,
                'current_I': 8,
                'optimal_W': 40,
                'optimal_I': 8,
                'min_I': 4,  # 整数位下限4-bit
                'optimized': False,
                'optimization_history': []
            }
        
        print(f"  找到 {len(self.variables)} 个typedef变量（已排除子函数变量）")
        
        # 显示前5个变量
        for idx, (var_name, config) in enumerate(list(self.variables.items())[:5]):
            print(f"    {idx+1}. {var_name}: W={config['init_W']}, I={config['init_I']}")
        if len(self.variables) > 5:
            print(f"    ... 还有 {len(self.variables) - 5} 个变量")
        
        return self.variables
    
    def generate_header_from_config(self, var_configs):
        """
        从变量配置生成头文件内容（不使用模板，直接生成）
        
        参数:
            var_configs: 变量配置字典
        
        返回:
            头文件内容字符串
        """
        header_lines = [
            "#pragma once",
            '#include "ap_fixed.h"',
            '#include "ap_int.h"',
            '#include "hls_math.h"',
            '#include "hls_stream.h"',
            "",
            "// 自动生成的优化位宽定义",
            ""
        ]
        
        # 按变量名排序以保持一致性
        sorted_vars = sorted(var_configs.items())
        
        # 生成typedef定义
        for var_name, config in sorted_vars:
            W = config.get('optimal_W', config.get('init_W', 40))
            I = config.get('optimal_I', config.get('init_I', 8))
            comment = f"// 变量: {var_name} (W={W}, I={I})"
            typedef_line = f"typedef ap_fixed<{W}, {I}> {var_name}_t;"
            header_lines.append(comment)
            header_lines.append(typedef_line)
            header_lines.append("")
        
        return '\n'.join(header_lines)
    
    def update_mhgd_header_params(self, mhgd_header_path, mimo_config):
        """
        更新MHGD_accel_hw.h中的MIMO参数
        
        参数:
            mhgd_header_path: MHGD_accel_hw.h路径
            mimo_config: MIMO配置 {'nt': 4, 'nr': 4, 'modulation': 16}
        """
        if not os.path.exists(mhgd_header_path):
            print(f"  警告: MHGD_accel_hw.h不存在: {mhgd_header_path}")
            return
        
        # 读取文件
        with open(mhgd_header_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 计算参数
        nt = mimo_config.get('nt', 8)
        modulation = mimo_config.get('modulation', 16)
        modulation_order = int(math.log2(modulation))
        
        # 更新参数
        content = re.sub(
            r'(static\s+const\s+int\s+Ntr_1\s*=\s*)\d+\s*;',
            rf'\g<1>{nt};',
            content
        )
        content = re.sub(
            r'(static\s+const\s+int\s+mu_1\s*=\s*)\d+\s*;',
            rf'\g<1>{modulation_order};',
            content
        )
        content = re.sub(
            r'(static\s+const\s+int\s+mu_double\s*=\s*)\d+\s*;',
            rf'\g<1>{modulation};',
            content
        )
        
        # 写回文件
        with open(mhgd_header_path, 'w', encoding='utf-8') as f:
            f.write(content)
        
        print(f"  更新MHGD_accel_hw.h参数:")
        print(f"    Ntr_1 = {nt}")
        print(f"    mu_1 = {modulation_order} (log2({modulation}))")
        print(f"    mu_double = {modulation}")
    
    def compile_and_test(self, config_name, var_configs, mimo_config, data_base_path=".", 
                        gaussian_noise_path="/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai"):
        """
        编译并测试当前位宽配置
        
        返回: BER值（失败返回None）
        """
        # 生成临时头文件
        temp_header_content = self.generate_header_from_config(var_configs)
        temp_header = "MyComplex_1_temp.h"
        
        with open(temp_header, 'w', encoding='utf-8') as f:
            f.write(temp_header_content)
        
        # 备份原始头文件
        backup_header = "MyComplex_1.h.bak"
        if os.path.exists("MyComplex_1.h"):
            os.rename("MyComplex_1.h", backup_header)
        os.rename(temp_header, "MyComplex_1.h")
        
        # 备份和更新MHGD_accel_hw.h
        mhgd_header = "MHGD_accel_hw.h"
        mhgd_backup = "MHGD_accel_hw.h.bak"
        if os.path.exists(mhgd_header):
            # 创建备份
            with open(mhgd_header, 'r') as f:
                mhgd_content = f.read()
            with open(mhgd_backup, 'w') as f:
                f.write(mhgd_content)
            
            # 更新参数
            self.update_mhgd_header_params(mhgd_header, mimo_config)
        
        try:
            # 生成TCL脚本
            nt = mimo_config.get('nt', 8)
            nr = mimo_config.get('nr', 8)
            modulation = mimo_config.get('modulation', 16)
            mimo_config_name = f"{nt}_{nr}_{modulation}QAM"
            
            tcl_file = generate_full_hls_tcl(
                config_name=config_name,
                data_base_path=data_base_path,
                mimo_config_name=mimo_config_name,
                gaussian_noise_path=gaussian_noise_path
            )
            
            # 运行Vitis HLS
            hls_cmd = ["vitis_hls", "-f", tcl_file]
            
            script_dir = os.path.dirname(os.path.abspath(__file__))
            logfiles_dir = os.path.join(script_dir, 'logfiles')
            log_file = os.path.join(logfiles_dir, f"build_{config_name}.log")
            
            with open(log_file, 'w') as log:
                result = subprocess.run(
                    hls_cmd,
                    stdout=log,
                    stderr=subprocess.STDOUT,
                    text=True,
                    timeout=600
                )
            
            # 提取BER
            with open(log_file, 'r') as log:
                log_content = log.read()
            
            ber_match = re.search(r'FINAL_BER:\s*([\d.e+-]+)', log_content)
            if not ber_match:
                ber_match = re.search(r'BER\s*[=:]\s*([\d.e+-]+)', log_content, re.IGNORECASE)
            
            if ber_match:
                ber = float(ber_match.group(1))
                return ber
            else:
                return None
        
        except Exception as e:
            print(f"    错误: {e}")
            return None
        
        finally:
            # 恢复原始头文件
            if os.path.exists("MyComplex_1.h"):
                os.remove("MyComplex_1.h")
            if os.path.exists(backup_header):
                os.rename(backup_header, "MyComplex_1.h")
            
            # 恢复MHGD_accel_hw.h
            if os.path.exists(mhgd_backup):
                if os.path.exists(mhgd_header):
                    os.remove(mhgd_header)
                os.rename(mhgd_backup, mhgd_header)
    
    def cleanup_variable_logs(self, var_name):
        """清理变量的中间日志文件"""
        script_dir = os.path.dirname(os.path.abspath(__file__))
        logfiles_dir = os.path.join(script_dir, 'logfiles')
        
        if not os.path.exists(logfiles_dir):
            return
        
        patterns = [
            f"run_hls_{var_name}_*.tcl",
            f"build_{var_name}_*.log"
        ]
        
        deleted_count = 0
        for pattern in patterns:
            full_pattern = os.path.join(logfiles_dir, pattern)
            for file_path in glob.glob(full_pattern):
                try:
                    os.remove(file_path)
                    deleted_count += 1
                except Exception as e:
                    print(f"  警告: 无法删除 {file_path}: {e}")
        
        if deleted_count > 0:
            print(f"  清理完成: 删除了{deleted_count}个日志文件")
    
    def get_baseline_ber(self, mimo_config, data_base_path=".", gaussian_noise_path="/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai"):
        """获取基准BER（使用初始位宽W=40, I=8）"""
        print("\n" + "="*70)
        print("  获取基准BER (W=40, I=8)")
        print("="*70)
        
        # 使用初始位宽配置
        baseline_config = {var: {'current_W': 40, 'current_I': 8} 
                          for var in self.variables.keys()}
        
        ber = self.compile_and_test("baseline", baseline_config, mimo_config, 
                                     data_base_path, gaussian_noise_path)
        
        if ber is None:
            raise RuntimeError("无法获取基准BER")
        
        self.baseline_ber = ber
        print(f"  基准BER = {ber:.8f}")
        print("="*70)
        
        return ber
    
    def optimize_variable_fractional(self, var_name, mimo_config, data_base_path=".", 
                                     gaussian_noise_path="/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai"):
        """
        优化变量的小数位宽（固定整数位=8）
        
        策略：
        1. 从40-bit开始，每次减少4-bit
        2. 当BER超过阈值时，回退并改为逐bit减少
        3. 找到最小可行小数位宽
        """
        print(f"\n{'='*70}")
        print(f"正在优化变量: {var_name} (小数位)")
        print(f"  初始位宽: W=40, I=8 (小数32位)")
        print("="*70)
        
        current_W = 40
        current_I = 8
        optimal_W = 40
        test_count = 0
        
        # 阶段1: 4-bit跳跃搜索
        print(f"\n  阶段1: 4-bit跳跃搜索")
        while current_W > current_I + 2:  # 至少保留2位小数
            test_W = current_W - 4
            if test_W < current_I + 2:
                test_W = current_I + 2
            
            test_count += 1
            config_name = f"{var_name}_W{test_W}"
            
            # 创建测试配置
            test_config = {v: {'current_W': self.variables[v]['optimal_W'], 
                              'current_I': self.variables[v]['optimal_I']}
                          for v in self.variables.keys()}
            test_config[var_name] = {'current_W': test_W, 'current_I': current_I}
            
            print(f"  [{test_count}] 测试位宽 W={test_W} (小数{test_W-current_I}位)...", end='', flush=True)
            
            ber = self.compile_and_test(config_name, test_config, mimo_config, 
                                        data_base_path, gaussian_noise_path)
            
            if ber is None:
                print(f" ✗ 测试失败")
                break
            
            ber_diff = abs(ber - self.baseline_ber)
            print(f" BER={ber:.8f}, 差异={ber_diff:.8f}")
            
            if ber <= self.ber_threshold:
                print(f"    ✓ 通过")
                optimal_W = test_W
                current_W = test_W
                # 检查是否已达到最小位宽
                if test_W == current_I + 2:
                    break
            else:
                print(f"    ✗ BER超出阈值，停止跳跃搜索")
                break
        
        # 阶段2: 如果跳跃搜索失败，从上一个成功值开始逐bit精细搜索
        if current_W > optimal_W:
            print(f"\n  阶段2: 从W={optimal_W}开始逐bit精细搜索")
            current_W = optimal_W
            
            while current_W > current_I + 2:
                test_W = current_W - 1
                
                test_count += 1
                config_name = f"{var_name}_W{test_W}_fine"
                
                test_config = {v: {'current_W': self.variables[v]['optimal_W'], 
                                  'current_I': self.variables[v]['optimal_I']}
                              for v in self.variables.keys()}
                test_config[var_name] = {'current_W': test_W, 'current_I': current_I}
                
                print(f"  [{test_count}] 测试位宽 W={test_W} (小数{test_W-current_I}位)...", end='', flush=True)
                
                ber = self.compile_and_test(config_name, test_config, mimo_config, 
                                            data_base_path, gaussian_noise_path)
                
                if ber is None:
                    print(f" ✗ 测试失败")
                    break
                
                ber_diff = abs(ber - self.baseline_ber)
                print(f" BER={ber:.8f}, 差异={ber_diff:.8f}")
                
                if ber <= self.ber_threshold:
                    print(f"    ✓ 通过")
                    optimal_W = test_W
                    current_W = test_W
                else:
                    print(f"    ✗ BER超出阈值，停止精细搜索")
                    break
        
        # 更新变量配置
        self.variables[var_name]['optimal_W'] = optimal_W
        self.variables[var_name]['optimal_I'] = current_I
        
        reduction = 40 - optimal_W
        print(f"\n  小数位优化完成:")
        print(f"    最优位宽: W={optimal_W}, I={current_I} (小数{optimal_W-current_I}位)")
        print(f"    位宽减少: {reduction} bits ({reduction/40*100:.1f}%)")
        print("="*70)
        
        return optimal_W, current_I
    
    def optimize_variable_integer(self, var_name, mimo_config, data_base_path=".", 
                                  gaussian_noise_path="/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai"):
        """
        优化变量的整数位宽（固定小数位=已优化值）
        
        策略：
        1. 从I=8开始，每次减少2-bit
        2. 下限为4-bit
        """
        current_W = self.variables[var_name]['optimal_W']
        current_I = 8
        optimal_I = 8
        fractional_bits = current_W - current_I
        
        print(f"\n{'='*70}")
        print(f"正在优化变量: {var_name} (整数位)")
        print(f"  当前位宽: W={current_W}, I=8 (小数{fractional_bits}位)")
        print("="*70)
        
        test_count = 0
        
        # 每次减少2-bit，下限4-bit
        while current_I > 4:
            test_I = current_I - 2
            if test_I < 4:
                test_I = 4
            
            test_W = test_I + fractional_bits
            test_count += 1
            config_name = f"{var_name}_I{test_I}"
            
            # 创建测试配置
            test_config = {v: {'current_W': self.variables[v]['optimal_W'], 
                              'current_I': self.variables[v]['optimal_I']}
                          for v in self.variables.keys()}
            test_config[var_name] = {'current_W': test_W, 'current_I': test_I}
            
            print(f"  [{test_count}] 测试位宽 W={test_W}, I={test_I}...", end='', flush=True)
            
            ber = self.compile_and_test(config_name, test_config, mimo_config, 
                                        data_base_path, gaussian_noise_path)
            
            if ber is None:
                print(f" ✗ 测试失败")
                break
            
            ber_diff = abs(ber - self.baseline_ber)
            print(f" BER={ber:.8f}, 差异={ber_diff:.8f}")
            
            if ber <= self.ber_threshold:
                print(f"    ✓ 通过")
                optimal_I = test_I
                current_I = test_I
                # 检查是否已达到最小整数位宽
                if test_I == 4:
                    break
            else:
                print(f"    ✗ BER超出阈值，停止优化")
                break
        
        # 更新变量配置
        optimal_W = optimal_I + fractional_bits
        self.variables[var_name]['optimal_W'] = optimal_W
        self.variables[var_name]['optimal_I'] = optimal_I
        
        print(f"\n  整数位优化完成:")
        print(f"    最优位宽: W={optimal_W}, I={optimal_I} (小数{fractional_bits}位)")
        print(f"    总减少: {40 - optimal_W} bits")
        print("="*70)
        
        return optimal_W, optimal_I
    
    def run_optimization(self, mimo_config, data_base_path=".", 
                        gaussian_noise_path="/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai", 
                        order='high_to_low'):
        """
        运行完整的位宽优化流程
        
        参数:
            mimo_config: MIMO配置
            data_base_path: 数据路径
            gaussian_noise_path: 高斯噪声路径
            order: 优化顺序 ('high_to_low' 或 'low_to_high')
        """
        print("\n" + "="*70)
        print("  开始位宽优化")
        print("="*70)
        
        # 解析头文件
        self.parse_header_typedef_only()
        
        # 获取基准BER
        self.get_baseline_ber(mimo_config, data_base_path, gaussian_noise_path)
        
        # 准备变量列表
        var_list = list(self.variables.keys())
        if order == 'low_to_high':
            var_list.reverse()
        
        total_vars = len(var_list)
        start_time = time.time()
        
        # 优化每个变量
        for idx, var_name in enumerate(var_list, 1):
            elapsed = time.time() - start_time
            if idx > 1:
                avg_time = elapsed / (idx - 1)
                remaining_vars = total_vars - idx + 1
                estimated_remaining = avg_time * remaining_vars
                
                print(f"\n{'█'*70}")
                print(f" 进度: [{idx}/{total_vars}] ({idx/total_vars*100:.1f}%)")
                print(f"{'█'*70}")
                print(f"⏱  已用时间: {elapsed/60:.1f}分钟")
                print(f"⏱  预计剩余: {estimated_remaining/60:.1f}分钟")
                print(f"⏱  预计总用时: {(elapsed+estimated_remaining)/60:.1f}分钟")
            
            # 优化小数位
            self.optimize_variable_fractional(var_name, mimo_config, data_base_path, gaussian_noise_path)
            
            # 优化整数位
            self.optimize_variable_integer(var_name, mimo_config, data_base_path, gaussian_noise_path)
            
            # 清理日志
            self.cleanup_variable_logs(var_name)
        
        # 增加2位小数以增强鲁棒性
        print(f"\n{'='*70}")
        print("  增强鲁棒性: 所有变量增加2位小数")
        print("="*70)
        
        for var_name in self.variables.keys():
            original_W = self.variables[var_name]['optimal_W']
            self.variables[var_name]['optimal_W'] = original_W + 2
            print(f"  {var_name}: W={original_W} → W={original_W + 2}")
        
        # 显示最终汇总
        self.print_final_summary()
        
        return self.variables
    
    def print_final_summary(self):
        """打印最终优化汇总"""
        print(f"\n{'='*80}")
        print(f"{'所有变量最优位宽汇总':^80}")
        print("="*80)
        print(f"{'变量名':<35} {'原始(W,I)':<15} {'优化后(W,I)':<15} {'减少':<10}")
        print("-"*80)
        
        total_reduction = 0
        for var_name, config in sorted(self.variables.items()):
            original_W = config['init_W']
            original_I = config['init_I']
            optimal_W = config['optimal_W']
            optimal_I = config['optimal_I']
            reduction = original_W - optimal_W
            total_reduction += reduction
            
            status = "✓"
            print(f"{status} {var_name:<33} ({original_W},{original_I}){'':<8} ({optimal_W},{optimal_I}){'':<8} {reduction} bits")
        
        print("-"*80)
        avg_reduction = total_reduction / len(self.variables) if self.variables else 0
        print(f"总位宽减少: {total_reduction} bits")
        print(f"平均减少: {avg_reduction:.1f} bits/变量 ({avg_reduction/40*100:.1f}%)")
        print("="*80)
    
    def save_results(self, output_config_file, output_header_file):
        """保存优化结果"""
        # 保存JSON配置
        result_data = {
            'timestamp': time.strftime('%Y-%m-%d %H:%M:%S'),
            'baseline_ber': self.baseline_ber,
            'ber_threshold': self.ber_threshold,
            'variables': {}
        }
        
        for var_name, config in self.variables.items():
            result_data['variables'][var_name] = {
                'initial_W': config['init_W'],
                'initial_I': config['init_I'],
                'optimal_W': config['optimal_W'],
                'optimal_I': config['optimal_I'],
                'reduction': config['init_W'] - config['optimal_W']
            }
        
        with open(output_config_file, 'w', encoding='utf-8') as f:
            json.dump(result_data, f, indent=2, ensure_ascii=False)
        
        print(f"\n配置已保存: {output_config_file}")
        
        # 生成优化后的头文件
        header_content = self.generate_header_from_config(self.variables)
        with open(output_header_file, 'w', encoding='utf-8') as f:
            f.write(header_content)
        
        print(f"头文件已生成: {output_header_file}")


def main():
    """主函数"""
    import argparse
    
    parser = argparse.ArgumentParser(description='MIMO HLS固定点位宽优化工具 V2')
    parser.add_argument('--header', default='MyComplex_1.h', help='MyComplex_1.h路径')
    parser.add_argument('--template', default='sensitivity_types.hpp.jinja2', help='Jinja2模板路径')
    parser.add_argument('--output-dir', default='bitwidth_result', help='输出目录')
    parser.add_argument('--ber-threshold', type=float, default=0.01, help='BER阈值')
    parser.add_argument('--nt', type=int, default=8, help='发送天线数')
    parser.add_argument('--nr', type=int, default=8, help='接收天线数')
    parser.add_argument('--modulation', type=int, default=16, choices=[4, 16, 64], help='调制阶数')
    parser.add_argument('--snr', type=float, default=25, help='信噪比(dB)')
    parser.add_argument('--data-path', default='/home/ggg_wufuqi/hls/MHGD/MHGD', help='测试数据路径')
    parser.add_argument('--gaussian-noise-path', default='/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai', help='高斯噪声文件路径')
    parser.add_argument('--order', default='high_to_low', choices=['high_to_low', 'low_to_high'], help='优化顺序')
    parser.add_argument('--output-config', help='输出JSON配置文件路径')
    parser.add_argument('--output-header', help='输出头文件路径')
    
    args = parser.parse_args()
    
    # MIMO配置
    mimo_config = {
        'nt': args.nt,
        'nr': args.nr,
        'modulation': args.modulation,
        'snr': args.snr,
        'name': f"{args.nt}_{args.nr}_{args.modulation}QAM"
    }
    
    # 创建优化器
    optimizer = OptimizedBitwidthOptimizer(
        header_file=args.header,
        template_file=args.template,
        output_dir=args.output_dir,
        ber_threshold=args.ber_threshold
    )
    
    # 运行优化
    optimizer.run_optimization(
        mimo_config=mimo_config,
        data_base_path=args.data_path,
        gaussian_noise_path=args.gaussian_noise_path,
        order=args.order
    )
    
    # 保存结果
    if args.output_config is None:
        args.output_config = os.path.join(args.output_dir, f"bitwidth_config_{mimo_config['name']}_SNR{args.snr}.json")
    if args.output_header is None:
        args.output_header = os.path.join(args.output_dir, f"MyComplex_optimized_{mimo_config['name']}_SNR{args.snr}.h")
    
    optimizer.save_results(args.output_config, args.output_header)
    
    print(f"\n{'='*70}")
    print("  优化完成!")
    print("="*70)


if __name__ == '__main__':
    main()
