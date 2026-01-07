#!/usr/bin/env python3
"""
固定点位宽自动优化脚本
基于BER性能逐步削减变量位宽，找到最优位宽配置
"""

import re
import os
import sys
import subprocess
import json
from pathlib import Path
from jinja2 import Template

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
        
    def parse_header_file(self):
        """解析头文件，提取所有变量及其位宽信息"""
        print(f"解析头文件: {self.header_file}")
        
        with open(self.header_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 匹配 typedef ap_fixed<W, I> var_name_t; 格式
        pattern = r'typedef\s+ap_fixed<(\d+),\s*(\d+)(?:,\s*[A-Z_]+)?(?:,\s*[A-Z_]+)?>\s+(\w+)_t;'
        matches = re.findall(pattern, content)
        
        for width, int_bits, var_name in matches:
            self.variables[var_name] = {
                'init_W': int(width),
                'init_I': int(int_bits),
                'current_W': int(width),
                'current_I': int(int_bits),
                'min_W': int(int_bits) + 2,  # 至少保留整数位+符号位+2位小数
                'optimized': False
            }
        
        print(f"  找到 {len(self.variables)} 个变量")
        for var_name, config in list(self.variables.items())[:5]:
            print(f"    {var_name}: W={config['init_W']}, I={config['init_I']}")
        if len(self.variables) > 5:
            print(f"    ... 还有 {len(self.variables) - 5} 个变量")
        
        return self.variables
    
    def generate_header_from_template(self, output_file, var_configs):
        """使用jinja2模板生成新的头文件"""
        with open(self.template_file, 'r', encoding='utf-8') as f:
            template_content = f.read()
        
        template = Template(template_content)
        rendered = template.render(vars=var_configs)
        
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(rendered)
    
    def compile_and_test(self, config_name, var_configs, mimo_config):
        """
        编译HLS项目并运行测试
        
        参数:
            config_name: 配置名称
            var_configs: 变量位宽配置
            mimo_config: MIMO测试配置 (Nt, Nr, modulation, SNR)
        
        返回:
            BER值，如果编译或测试失败返回None
        """
        print(f"  测试配置: {config_name}")
        
        # 生成临时头文件
        temp_header = "MyComplex_1_temp.h"
        self.generate_header_from_template(temp_header, var_configs)
        
        # 备份原始头文件
        os.rename("MyComplex_1.h", "MyComplex_1.h.bak")
        os.rename(temp_header, "MyComplex_1.h")
        
        try:
            # 编译 (假设使用vitis_hls或者简单的g++编译testbench)
            # 这里需要根据实际的编译流程调整
            compile_cmd = ["g++", "-std=c++11", "-I.", "-o", "test_mimo", 
                          "main_hw.cpp", "MHGD_accel_hw.cpp",
                          "-lm"]
            
            result = subprocess.run(compile_cmd, capture_output=True, text=True, timeout=120)
            
            if result.returncode != 0:
                print(f"    编译失败: {result.stderr[:200]}")
                return None
            
            # 运行测试
            test_cmd = ["./test_mimo"]
            result = subprocess.run(test_cmd, capture_output=True, text=True, timeout=300)
            
            if result.returncode != 0:
                print(f"    运行失败: {result.stderr[:200]}")
                return None
            
            # 从输出中提取BER
            output = result.stdout
            ber_match = re.search(r'FINAL_BER:\s*([\d.e+-]+)', output)
            
            if ber_match:
                ber = float(ber_match.group(1))
                print(f"    BER = {ber:.8f}")
                return ber
            else:
                print(f"    无法提取BER")
                return None
                
        except subprocess.TimeoutExpired:
            print(f"    执行超时")
            return None
        except Exception as e:
            print(f"    发生错误: {e}")
            return None
        finally:
            # 恢复原始头文件
            os.rename("MyComplex_1.h", temp_header)
            os.rename("MyComplex_1.h.bak", "MyComplex_1.h")
            # 清理临时文件
            if os.path.exists(temp_header):
                os.remove(temp_header)
            if os.path.exists("test_mimo"):
                os.remove("test_mimo")
    
    def get_baseline_ber(self, mimo_config):
        """获取64位全精度基准BER"""
        print("获取64位全精度基准BER...")
        
        # 使用当前(全精度)配置
        baseline_config = {var: self.variables[var] for var in self.variables}
        
        ber = self.compile_and_test("baseline", baseline_config, mimo_config)
        
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
        
        # 从当前位宽开始，逐步减小
        for test_W in range(current_W, min_W - 1, -step):
            # 构建测试配置
            test_config = {var: self.variables[var].copy() for var in self.variables}
            test_config[var_name]['current_W'] = test_W
            
            # 测试此配置
            ber = self.compile_and_test(f"{var_name}_W{test_W}", test_config, mimo_config)
            
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
    
    parser = argparse.ArgumentParser(description='MIMO HLS位宽自动优化工具')
    parser.add_argument('--header', default='MyComplex_1.h', 
                       help='输入头文件路径')
    parser.add_argument('--template', default='sensitivity_types.hpp.jinja2',
                       help='Jinja2模板文件路径')
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
    
    args = parser.parse_args()
    
    # 构建MIMO配置
    mimo_config = {
        'Nt': args.nt,
        'Nr': args.nr,
        'modulation': args.modulation,
        'SNR': args.snr
    }
    
    # 默认输出文件名
    if args.output_config is None:
        args.output_config = f"bitwidth_config_{args.nt}x{args.nr}_{args.modulation}QAM_SNR{args.snr}.json"
    
    if args.output_header is None:
        args.output_header = f"MyComplex_optimized_{args.nt}x{args.nr}_{args.modulation}QAM_SNR{args.snr}.h"
    
    # 创建优化器
    optimizer = BitwidthOptimizer(args.header, args.template, args.ber_threshold)
    
    # 解析头文件
    optimizer.parse_header_file()
    
    # 执行优化
    optimizer.optimize_all_variables(mimo_config, args.order)
    
    # 保存结果
    optimizer.save_optimized_config(args.output_config, mimo_config)
    optimizer.generate_optimized_header(args.output_header)
    
    print("\n优化流程完成！")

if __name__ == "__main__":
    main()
