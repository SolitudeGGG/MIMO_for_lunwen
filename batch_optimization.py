#!/usr/bin/env python3
"""
批量位宽优化脚本
为不同MIMO规模、调制阶数和SNR生成优化的头文件
"""

import os
import sys
import subprocess
from itertools import product

def run_optimization(nt, nr, modulation, snr, output_dir="optimized_headers"):
    """
    运行单个配置的位宽优化
    
    参数:
        nt: 发送天线数
        nr: 接收天线数  
        modulation: 调制阶数 (4, 16, 64)
        snr: 信噪比(dB)
        output_dir: 输出目录
    """
    os.makedirs(output_dir, exist_ok=True)
    
    config_name = f"{nt}x{nr}_{modulation}QAM_SNR{snr}"
    output_config = os.path.join(output_dir, f"bitwidth_config_{config_name}.json")
    output_header = os.path.join(output_dir, f"MyComplex_optimized_{config_name}.h")
    
    print("\n" + "=" * 80)
    print(f"优化配置: {config_name}")
    print("=" * 80)
    
    cmd = [
        "python3", "bitwidth_optimization.py",
        "--nt", str(nt),
        "--nr", str(nr),
        "--modulation", str(modulation),
        "--snr", str(snr),
        "--output-config", output_config,
        "--output-header", output_header,
        "--order", "high_to_low"
    ]
    
    try:
        result = subprocess.run(cmd, check=True, capture_output=True, text=True)
        print(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        print(f"优化失败: {e}")
        print(f"输出: {e.stdout}")
        print(f"错误: {e.stderr}")
        return False

def main():
    """主函数：批量运行所有配置的优化"""
    
    # 配置参数
    antenna_configs = [(8, 8)]  # 可以扩展: [(4, 4), (8, 8), (16, 16)]
    modulation_orders = [4, 16, 64]
    snr_values = [5, 10, 15, 20, 25]
    
    # 生成所有配置组合
    configs = list(product(antenna_configs, modulation_orders, snr_values))
    
    print("=" * 80)
    print("批量位宽优化工具")
    print("=" * 80)
    print(f"总共需要优化 {len(configs)} 个配置")
    print()
    
    # 记录结果
    results = {
        'success': [],
        'failed': []
    }
    
    # 执行优化
    for idx, ((nt, nr), modulation, snr) in enumerate(configs, 1):
        print(f"\n进度: [{idx}/{len(configs)}]")
        
        success = run_optimization(nt, nr, modulation, snr)
        
        config_name = f"{nt}x{nr}_{modulation}QAM_SNR{snr}"
        if success:
            results['success'].append(config_name)
        else:
            results['failed'].append(config_name)
    
    # 打印摘要
    print("\n" + "=" * 80)
    print("批量优化完成")
    print("=" * 80)
    print(f"成功: {len(results['success'])} 个配置")
    print(f"失败: {len(results['failed'])} 个配置")
    
    if results['failed']:
        print("\n失败的配置:")
        for config in results['failed']:
            print(f"  - {config}")
    
    print("\n所有优化后的头文件保存在: optimized_headers/")

if __name__ == "__main__":
    main()
