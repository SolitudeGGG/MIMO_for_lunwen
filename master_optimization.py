#!/usr/bin/env python3
"""
主控脚本 - 完整的MIMO位宽优化流程
1. 生成MIMO测试数据
2. 运行基准测试
3. 执行位宽优化
4. 生成多个配置的优化头文件
5. 分析子函数变量
"""

import os
import sys
import subprocess
import argparse
from pathlib import Path

def check_dependencies():
    """检查依赖"""
    print("检查依赖...")
    
    # 检查Python库
    required_libs = ['numpy', 'jinja2']
    missing_libs = []
    
    for lib in required_libs:
        try:
            __import__(lib)
        except ImportError:
            missing_libs.append(lib)
    
    # 检查commpy
    try:
        import commpy
    except ImportError:
        missing_libs.append('scikit-commpy')
    
    if missing_libs:
        print(f"缺少依赖: {', '.join(missing_libs)}")
        print("请运行: pip install " + " ".join(missing_libs))
        return False
    
    print("  依赖检查通过")
    return True

def step1_generate_mimo_data():
    """步骤1: 生成MIMO测试数据"""
    print("\n" + "=" * 80)
    print("步骤 1: 生成MIMO测试数据")
    print("=" * 80)
    
    if not os.path.exists("generate_mimo_data.py"):
        print("错误: generate_mimo_data.py 不存在")
        return False
    
    try:
        result = subprocess.run(["python3", "generate_mimo_data.py"], 
                              check=True, capture_output=True, text=True)
        print(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        print(f"数据生成失败: {e}")
        print(e.stdout)
        print(e.stderr)
        return False

def step2_run_baseline_test(config):
    """步骤2: 运行基准测试"""
    print("\n" + "=" * 80)
    print("步骤 2: 运行64位全精度基准测试")
    print("=" * 80)
    
    # 编译并运行基准测试
    try:
        # 这里需要根据实际的编译流程调整
        print("编译测试程序...")
        compile_cmd = ["g++", "-std=c++11", "-I.", "-o", "baseline_test",
                      "main_hw.cpp", "MHGD_accel_hw.cpp", "-lm"]
        
        result = subprocess.run(compile_cmd, check=True, capture_output=True, text=True)
        print("  编译成功")
        
        print("运行基准测试...")
        result = subprocess.run(["./baseline_test"], 
                              check=True, capture_output=True, text=True, timeout=300)
        print(result.stdout)
        
        # 清理
        if os.path.exists("baseline_test"):
            os.remove("baseline_test")
        
        return True
    except subprocess.CalledProcessError as e:
        print(f"基准测试失败: {e}")
        print(e.stderr)
        return False
    except subprocess.TimeoutExpired:
        print("基准测试超时")
        return False

def step3_analyze_subfunctions():
    """步骤3: 分析子函数变量"""
    print("\n" + "=" * 80)
    print("步骤 3: 分析子函数变量")
    print("=" * 80)
    
    if not os.path.exists("subfunction_analyzer.py"):
        print("错误: subfunction_analyzer.py 不存在")
        return False
    
    try:
        result = subprocess.run(["python3", "subfunction_analyzer.py"],
                              check=True, capture_output=True, text=True)
        print(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        print(f"子函数分析失败: {e}")
        print(e.stderr)
        return False

def step4_optimize_bitwidths(mode='single'):
    """步骤4: 执行位宽优化"""
    print("\n" + "=" * 80)
    print("步骤 4: 执行位宽优化")
    print("=" * 80)
    
    if mode == 'batch':
        # 批量优化
        if not os.path.exists("batch_optimization.py"):
            print("错误: batch_optimization.py 不存在")
            return False
        
        try:
            result = subprocess.run(["python3", "batch_optimization.py"],
                                  check=True, capture_output=True, text=True)
            print(result.stdout)
            return True
        except subprocess.CalledProcessError as e:
            print(f"批量优化失败: {e}")
            print(e.stderr)
            return False
    else:
        # 单配置优化示例
        if not os.path.exists("bitwidth_optimization.py"):
            print("错误: bitwidth_optimization.py 不存在")
            return False
        
        # 运行一个示例配置: 8x8, 16QAM, SNR=20
        try:
            cmd = [
                "python3", "bitwidth_optimization.py",
                "--nt", "8", "--nr", "8",
                "--modulation", "16",
                "--snr", "20",
                "--order", "high_to_low"
            ]
            result = subprocess.run(cmd, check=True, capture_output=True, text=True)
            print(result.stdout)
            return True
        except subprocess.CalledProcessError as e:
            print(f"位宽优化失败: {e}")
            print(e.stderr)
            return False

def step5_generate_summary():
    """步骤5: 生成总结报告"""
    print("\n" + "=" * 80)
    print("步骤 5: 生成总结报告")
    print("=" * 80)
    
    # 收集所有优化结果
    optimized_dir = Path("optimized_headers")
    
    if not optimized_dir.exists():
        print("未找到优化结果目录")
        return False
    
    # 统计优化结果
    json_files = list(optimized_dir.glob("*.json"))
    header_files = list(optimized_dir.glob("*.h"))
    
    print(f"\n总结:")
    print(f"  生成的配置文件: {len(json_files)} 个")
    print(f"  生成的头文件: {len(header_files)} 个")
    
    if json_files:
        print(f"\n配置文件列表:")
        for f in sorted(json_files)[:10]:
            print(f"    - {f.name}")
        if len(json_files) > 10:
            print(f"    ... 还有 {len(json_files) - 10} 个")
    
    if header_files:
        print(f"\n优化后的头文件:")
        for f in sorted(header_files)[:10]:
            print(f"    - {f.name}")
        if len(header_files) > 10:
            print(f"    ... 还有 {len(header_files) - 10} 个")
    
    return True

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='MIMO HLS位宽优化主控脚本')
    parser.add_argument('--skip-data-gen', action='store_true',
                       help='跳过数据生成步骤')
    parser.add_argument('--skip-baseline', action='store_true',
                       help='跳过基准测试步骤')
    parser.add_argument('--skip-subfunction', action='store_true',
                       help='跳过子函数分析步骤')
    parser.add_argument('--optimization-mode', choices=['single', 'batch'], 
                       default='single',
                       help='优化模式: single=单配置, batch=批量')
    parser.add_argument('--steps', nargs='+', type=int,
                       help='只运行指定步骤 (1-5)')
    
    args = parser.parse_args()
    
    print("=" * 80)
    print("MIMO HLS 位宽优化自动化工具")
    print("=" * 80)
    print()
    
    # 检查依赖
    if not check_dependencies():
        return 1
    
    # 确定要运行的步骤
    if args.steps:
        steps_to_run = set(args.steps)
    else:
        steps_to_run = {1, 2, 3, 4, 5}
    
    success = True
    
    # 步骤1: 生成MIMO数据
    if 1 in steps_to_run and not args.skip_data_gen:
        if not step1_generate_mimo_data():
            print("\n错误: 数据生成失败")
            success = False
    
    # 步骤2: 基准测试
    if success and 2 in steps_to_run and not args.skip_baseline:
        if not step2_run_baseline_test(None):
            print("\n警告: 基准测试失败，但继续执行")
    
    # 步骤3: 分析子函数
    if success and 3 in steps_to_run and not args.skip_subfunction:
        if not step3_analyze_subfunctions():
            print("\n警告: 子函数分析失败，但继续执行")
    
    # 步骤4: 位宽优化
    if success and 4 in steps_to_run:
        if not step4_optimize_bitwidths(args.optimization_mode):
            print("\n错误: 位宽优化失败")
            success = False
    
    # 步骤5: 生成总结
    if success and 5 in steps_to_run:
        step5_generate_summary()
    
    # 最终总结
    print("\n" + "=" * 80)
    if success:
        print("所有步骤执行完成！")
        print("\n生成的文件:")
        print("  - mimo_data/          : MIMO测试数据")
        print("  - optimized_headers/  : 优化后的头文件和配置")
        print("  - subfunction_analysis_report.json : 子函数分析报告")
    else:
        print("执行过程中出现错误，请查看上面的日志")
    print("=" * 80)
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
