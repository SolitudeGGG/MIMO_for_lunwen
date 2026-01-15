#!/usr/bin/env python3
"""
MIMO HLS定点位宽优化脚本 - 防止假性优化版本

核心改进：
1. 编译验证机制：每次测试前验证代码能否编译
2. 保守优化策略：更小的步长和更高的下限
3. 严格BER阈值：默认0.005而非0.01
4. 详细错误日志：记录所有编译和测试失败

作者：GitHub Copilot
日期：2026-01-15
"""

import os
import sys
import json
import subprocess
import re
import shutil
import argparse
from pathlib import Path
from datetime import datetime
import math

class BitwithOptimizer:
    def __init__(self, nt, nr, modulation, snr, 
                 header_file='MyComplex_1.h',
                 mhgd_header='MHGD_accel_hw.h',
                 template_file='PYTHON_COPILOT/sensitivity_types.hpp.jinja2',
                 ber_threshold=0.005,  # 更严格的默认阈值
                 initial_W=40,
                 initial_I=8,
                 min_I=6):  # 更高的整数位下限
        
        self.nt = nt
        self.nr = nr
        self.modulation = modulation
        self.snr = snr
        self.header_file = header_file
        self.mhgd_header = mhgd_header
        self.template_file = template_file
        self.ber_threshold = ber_threshold
        self.initial_W = initial_W
        self.initial_I = initial_I
        self.min_I = min_I  # 保留更多整数位
        
        self.baseline_ber = None
        self.optimization_results = {}
        self.compilation_errors = []
        
        # 创建输出目录
        self.output_dir = Path('PYTHON_COPILOT/Newpython_result')
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        self.logdir = Path('PYTHON_COPILOT/logfiles')
        self.logdir.mkdir(parents=True, exist_ok=True)
        
        print(f"=== 位宽优化系统（防假性优化版） ===")
        print(f"MIMO配置: {nt}×{nr} {modulation}QAM, SNR={snr}dB")
        print(f"BER阈值: {ber_threshold} (严格模式)")
        print(f"初始位宽: W={initial_W}, I={initial_I}")
        print(f"整数位下限: I={min_I}")
        print(f"优化步长: 小数2bit, 整数1bit (保守)")
        print("=" * 70)
    
    def parse_header_variables(self):
        """解析头文件，只提取typedef变量"""
        print("\n[1/6] 解析头文件变量...")
        
        if not os.path.exists(self.header_file):
            print(f"错误：找不到头文件 {self.header_file}")
            sys.exit(1)
        
        with open(self.header_file, 'r') as f:
            content = f.read()
        
        # 只匹配typedef定义，排除子函数变量
        pattern = r'typedef\s+ap_fixed<(\d+),\s*(\d+)>\s+(\w+)_t;'
        matches = re.findall(pattern, content)
        
        variables = {}
        for W, I, var_name in matches:
            # 排除子函数局部变量（_subfunc_前缀）
            if not var_name.startswith('_subfunc_'):
                variables[var_name] = {
                    'init_W': int(W),
                    'init_I': int(I),
                    'current_W': int(W),
                    'current_I': int(I)
                }
        
        print(f"找到 {len(variables)} 个typedef变量（已排除子函数变量）")
        return variables
    
    def verify_compilation(self, test_name="test"):
        """验证当前配置能否编译通过"""
        print(f"  验证编译...")
        
        # 生成测试TCL
        tcl_file = self.logdir / f'verify_{test_name}.tcl'
        tcl_content = f"""
# 编译验证TCL
open_project -reset verify_{test_name}_proj
set_top MHGD_accel_hw
add_files MHGD_accel_hw.cpp
add_files MHGD_accel_hw.h
add_files MyComplex_1.h
open_solution -reset solution1
set_part {{xczu9eg-ffvb1156-2-e}}
create_clock -period 10
# 只做C语法检查，不运行Csim
csynth_design
close_project
exit
"""
        with open(tcl_file, 'w') as f:
            f.write(tcl_content)
        
        # 运行HLS验证
        log_file = self.logdir / f'verify_{test_name}.log'
        try:
            result = subprocess.run(
                ['vitis_hls', '-f', str(tcl_file)],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=300,  # 5分钟超时
                text=True
            )
            
            with open(log_file, 'w') as f:
                f.write(result.stdout)
            
            # 检查是否有编译错误
            if 'ERROR' in result.stdout or result.returncode != 0:
                # 提取错误信息
                error_lines = [line for line in result.stdout.split('\n') 
                              if 'error:' in line.lower()]
                self.compilation_errors.extend(error_lines[:5])  # 只保留前5个错误
                print(f"  ✗ 编译失败")
                return False
            
            print(f"  ✓ 编译通过")
            return True
            
        except subprocess.TimeoutExpired:
            print(f"  ✗ 编译超时")
            return False
        except Exception as e:
            print(f"  ✗ 编译验证异常: {e}")
            return False
        finally:
            # 清理临时项目
            if os.path.exists(f'verify_{test_name}_proj'):
                shutil.rmtree(f'verify_{test_name}_proj', ignore_errors=True)
    
    def run_hls_csim(self, test_name):
        """运行HLS C仿真并提取BER"""
        # 生成TCL脚本
        tcl_file = self.logdir / f'run_hls_{test_name}.tcl'
        tcl_content = self._generate_tcl(test_name)
        
        with open(tcl_file, 'w') as f:
            f.write(tcl_content)
        
        # 运行HLS
        log_file = self.logdir / f'build_{test_name}.log'
        try:
            result = subprocess.run(
                ['vitis_hls', '-f', str(tcl_file)],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=600,
                text=True
            )
            
            with open(log_file, 'w') as f:
                f.write(result.stdout)
            
            # 提取BER
            ber = self._extract_ber(result.stdout)
            return ber
            
        except subprocess.TimeoutExpired:
            print(f"  ✗ HLS超时")
            return None
        except Exception as e:
            print(f"  ✗ HLS异常: {e}")
            return None
    
    def _generate_tcl(self, test_name):
        """生成HLS TCL脚本"""
        mimo_config = f"{self.nt}_{self.nr}_{self.modulation}QAM"
        data_path = f"/home/ggg_wufuqi/hls/MHGD/MHGD/{mimo_config}"
        
        tcl = f"""
open_project -reset {test_name}_proj
set_top MHGD_accel_hw
add_files MHGD_accel_hw.cpp
add_files MHGD_accel_hw.h
add_files MyComplex_1.h
add_files -tb main_hw.cpp
open_solution -reset solution1
set_part {{xczu9eg-ffvb1156-2-e}}
create_clock -period 10
csim_design -argv "{self.snr} {data_path}"
close_project
exit
"""
        return tcl
    
    def _extract_ber(self, log_content):
        """从日志中提取BER值"""
        patterns = [
            r'BER\s*[=:]\s*([0-9.e-]+)',
            r'Bit Error Rate\s*[=:]\s*([0-9.e-]+)',
            r'BER_final\s*[=:]\s*([0-9.e-]+)'
        ]
        
        for pattern in patterns:
            match = re.search(pattern, log_content, re.IGNORECASE)
            if match:
                try:
                    return float(match.group(1))
                except:
                    pass
        
        return None
    
    def optimize_variable(self, var_name, var_config):
        """优化单个变量的位宽"""
        print(f"\n{'='*70}")
        print(f"正在优化变量: {var_name}")
        print(f"  初始位宽: W={var_config['init_W']}, I={var_config['init_I']}")
        print(f"{'='*70}")
        
        # 阶段1：小数位优化（保守策略：每次减少2 bits）
        optimal_W = var_config['init_W']
        optimal_I = var_config['init_I']
        
        print(f"\n阶段1: 小数位优化（每次减少2 bits）")
        current_W = var_config['init_W']
        
        while current_W > optimal_I + 2:
            test_W = current_W - 2
            if test_W < optimal_I + 2:
                break
            
            print(f"  [测试] W={test_W}, I={optimal_I} ...", end='', flush=True)
            
            # 更新配置并生成头文件
            test_config = var_config.copy()
            test_config['current_W'] = test_W
            test_config['current_I'] = optimal_I
            self._update_config_and_generate(var_name, test_config)
            
            # 验证编译
            if not self.verify_compilation(f"{var_name}_W{test_W}"):
                print(f" 编译失败，停止优化")
                break
            
            # 运行HLS测试
            ber = self.run_hls_csim(f"{var_name}_W{test_W}")
            
            if ber is None:
                print(f" BER提取失败")
                break
            
            if ber <= self.ber_threshold:
                print(f" ✓ BER={ber:.8f}")
                optimal_W = test_W
                current_W = test_W
            else:
                print(f" ✗ BER={ber:.8f} 超出阈值")
                break
        
        # 阶段2：整数位优化（保守策略：每次减少1 bit）
        print(f"\n阶段2: 整数位优化（每次减少1 bit，下限{self.min_I}）")
        
        test_I = optimal_I - 1
        while test_I >= self.min_I:
            print(f"  [测试] W={optimal_W}, I={test_I} ...", end='', flush=True)
            
            test_config = var_config.copy()
            test_config['current_W'] = optimal_W
            test_config['current_I'] = test_I
            self._update_config_and_generate(var_name, test_config)
            
            if not self.verify_compilation(f"{var_name}_I{test_I}"):
                print(f" 编译失败")
                break
            
            ber = self.run_hls_csim(f"{var_name}_I{test_I}")
            
            if ber is None:
                print(f" BER提取失败")
                break
            
            if ber <= self.ber_threshold:
                print(f" ✓ BER={ber:.8f}")
                optimal_I = test_I
                test_I -= 1
            else:
                print(f" ✗ BER={ber:.8f} 超出阈值")
                break
        
        # 阶段3：鲁棒性增强（+3位小数）
        print(f"\n阶段3: 鲁棒性增强（+3位小数）")
        final_W = optimal_W + 3
        final_I = optimal_I
        
        print(f"  最终位宽: W={final_W}, I={final_I}")
        print(f"  位宽减少: {var_config['init_W'] - final_W} bits")
        
        return {
            'optimal_W': final_W,
            'optimal_I': final_I,
            'init_W': var_config['init_W'],
            'init_I': var_config['init_I'],
            'reduction': var_config['init_W'] - final_W
        }
    
    def _update_config_and_generate(self, var_name, config):
        """更新配置并生成头文件"""
        temp_config = {var_name: config}
        self._generate_header(temp_config)
    
    def _generate_header(self, variables):
        """生成优化后的头文件"""
        header_content = """#pragma once
#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_math.h"
#include "hls_stream.h"

// 自动生成的优化位宽定义
// 生成时间: """ + datetime.now().strftime("%Y-%m-%d %H:%M:%S") + "\n\n"
        
        for var_name, config in variables.items():
            W = config.get('current_W', config.get('optimal_W', config['init_W']))
            I = config.get('current_I', config.get('optimal_I', config['init_I']))
            header_content += f"typedef ap_fixed<{W}, {I}> {var_name}_t;\n"
        
        with open(self.header_file, 'w') as f:
            f.write(header_content)
    
    def run(self):
        """运行完整优化流程"""
        print("\n开始位宽优化...")
        
        # 解析变量
        variables = self.parse_header_variables()
        
        # 获取基准BER
        print("\n[2/6] 获取基准BER...")
        self.baseline_ber = self.run_hls_csim("baseline")
        if self.baseline_ber is None:
            print("错误：无法获取基准BER")
            sys.exit(1)
        print(f"基准BER: {self.baseline_ber:.8f}")
        
        # 优化每个变量
        print("\n[3/6] 开始优化变量...")
        for i, (var_name, var_config) in enumerate(variables.items(), 1):
            print(f"\n进度: [{i}/{len(variables)}]")
            result = self.optimize_variable(var_name, var_config)
            self.optimization_results[var_name] = result
        
        # 生成最终配置
        print("\n[4/6] 生成最终配置...")
        final_config = {
            name: {
                'optimal_W': res['optimal_W'],
                'optimal_I': res['optimal_I'],
                'init_W': res['init_W'],
                'init_I': res['init_I']
            }
            for name, res in self.optimization_results.items()
        }
        self._generate_header(final_config)
        
        # 保存结果
        print("\n[5/6] 保存优化结果...")
        config_name = f"{self.nt}_{self.nr}_{self.modulation}QAM_SNR{self.snr}"
        json_file = self.output_dir / f'bitwidth_config_{config_name}.json'
        
        output_data = {
            'mimo_config': {'nt': self.nt, 'nr': self.nr, 'modulation': self.modulation, 'snr': self.snr},
            'baseline_ber': self.baseline_ber,
            'ber_threshold': self.ber_threshold,
            'variables': self.optimization_results,
            'compilation_errors': self.compilation_errors,
            'timestamp': datetime.now().isoformat()
        }
        
        with open(json_file, 'w') as f:
            json.dump(output_data, f, indent=2)
        
        # 复制优化后的头文件
        optimized_header = self.output_dir / f'MyComplex_optimized_{config_name}.h'
        shutil.copy(self.header_file, optimized_header)
        
        print(f"✓ 配置文件: {json_file}")
        print(f"✓ 优化头文件: {optimized_header}")
        
        # 生成总结
        print("\n[6/6] 优化总结")
        print("=" * 70)
        total_reduction = sum(res['reduction'] for res in self.optimization_results.values())
        avg_reduction = total_reduction / len(self.optimization_results)
        print(f"总位宽减少: {total_reduction} bits")
        print(f"平均减少: {avg_reduction:.1f} bits/变量")
        print(f"编译错误数: {len(self.compilation_errors)}")
        print("=" * 70)
        
        if self.compilation_errors:
            print("\n⚠️ 警告：优化过程中发现编译错误")
            print("请查看 CODE_MODIFICATIONS_GUIDE.md 了解如何修复")

def main():
    parser = argparse.ArgumentParser(description='MIMO HLS位宽优化（防假性优化版）')
    parser.add_argument('--nt', type=int, required=True, help='发送天线数')
    parser.add_argument('--nr', type=int, required=True, help='接收天线数')
    parser.add_argument('--modulation', type=int, required=True, choices=[4, 16, 64], help='调制阶数')
    parser.add_argument('--snr', type=float, required=True, help='信噪比(dB)')
    parser.add_argument('--ber-threshold', type=float, default=0.005, help='BER阈值（默认0.005）')
    parser.add_argument('--initial-W', type=int, default=40, help='初始总位宽')
    parser.add_argument('--initial-I', type=int, default=8, help='初始整数位宽')
    parser.add_argument('--min-I', type=int, default=6, help='最小整数位宽')
    
    args = parser.parse_args()
    
    optimizer = BitwithOptimizer(
        nt=args.nt,
        nr=args.nr,
        modulation=args.modulation,
        snr=args.snr,
        ber_threshold=args.ber_threshold,
        initial_W=args.initial_W,
        initial_I=args.initial_I,
        min_I=args.min_I
    )
    
    optimizer.run()

if __name__ == '__main__':
    main()
