#!/usr/bin/env python3
"""
批量MIMO位宽优化脚本
自动对多种MIMO配置执行位宽优化

支持的MIMO配置:
1. 4x4, 16QAM
2. 8x8, 4QAM
3. 8x8, 16QAM
4. 8x8, 64QAM
5. 16x16, 16QAM
6. 16x16, 64QAM
7. 32x32, 16QAM
8. 64x64, 16QAM

作者: GitHub Copilot
日期: 2026-01-07
"""

import os
import sys
import json
import subprocess
import argparse
from datetime import datetime
from pathlib import Path

class BatchBitwidthOptimizer:
    def __init__(self, config):
        """
        初始化批量优化器
        
        参数:
            config: 全局配置字典
        """
        self.config = config
        self.results = {}
        self.failed_configs = []
        
    def get_mimo_configs(self):
        """
        定义所有需要优化的MIMO配置
        
        返回:
            MIMO配置列表
        """
        configs = [
            {'nt': 4, 'nr': 4, 'modulation': 16, 'snr': 25, 'name': '4_4_16QAM'},
            {'nt': 8, 'nr': 8, 'modulation': 4, 'snr': 25, 'name': '8_8_4QAM'},
            {'nt': 8, 'nr': 8, 'modulation': 16, 'snr': 25, 'name': '8_8_16QAM'},
            {'nt': 8, 'nr': 8, 'modulation': 64, 'snr': 25, 'name': '8_8_64QAM'},
            {'nt': 16, 'nr': 16, 'modulation': 16, 'snr': 25, 'name': '16_16_16QAM'},
            {'nt': 16, 'nr': 16, 'modulation': 64, 'snr': 25, 'name': '16_16_64QAM'},
            {'nt': 32, 'nr': 32, 'modulation': 16, 'snr': 25, 'name': '32_32_16QAM'},
            {'nt': 64, 'nr': 64, 'modulation': 16, 'snr': 25, 'name': '64_64_16QAM'},
        ]
        return configs
    
    def run_single_optimization(self, mimo_config):
        """
        运行单个MIMO配置的位宽优化
        
        参数:
            mimo_config: MIMO配置字典
        
        返回:
            (success, result_info)
        """
        print("\n" + "="*80)
        print(f"开始优化配置: {mimo_config['name']}")
        print(f"  天线数: {mimo_config['nt']}x{mimo_config['nr']}")
        print(f"  调制方式: {mimo_config['modulation']}QAM")
        print(f"  SNR: {mimo_config['snr']}dB")
        print("="*80)
        
        # 构建输出文件路径
        output_dir = Path(self.config['output_dir'])
        output_dir.mkdir(parents=True, exist_ok=True)
        
        output_config = output_dir / f"bitwidth_config_{mimo_config['name']}_SNR{mimo_config['snr']}.json"
        output_header = output_dir / f"MyComplex_optimized_{mimo_config['name']}_SNR{mimo_config['snr']}.h"
        
        # 构建命令 - 使用绝对路径确保脚本能被找到
        import os
        script_dir = os.path.dirname(os.path.abspath(__file__))
        script_path = os.path.join(script_dir, 'bitwidth_optimization.py')
        
        cmd = [
            'python3', script_path,
            '--header', self.config['header_file'],
            '--template', self.config['template_file'],
            '--ber-threshold', str(self.config['ber_threshold']),
            '--nt', str(mimo_config['nt']),
            '--nr', str(mimo_config['nr']),
            '--modulation', str(mimo_config['modulation']),
            '--snr', str(mimo_config['snr']),
            '--data-path', self.config['data_path'],
            '--gaussian-noise-path', self.config['gaussian_noise_path'],
            '--output-config', str(output_config),
            '--output-header', str(output_header),
            '--order', self.config['optimization_order'],
            '--initial-W', str(self.config.get('initial_W', 40)),
            '--initial-I', str(self.config.get('initial_I', 8))
        ]
        
        print(f"\n执行命令:")
        # 使用shlex.quote来正确显示带空格的参数
        import shlex
        print(f"  {' '.join(shlex.quote(arg) for arg in cmd)}")
        print()
        
        # 记录开始时间
        start_time = datetime.now()
        
        try:
            # 执行优化脚本
            result = subprocess.run(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                timeout=7200  # 2小时超时
            )
            
            # 计算运行时间
            end_time = datetime.now()
            duration = (end_time - start_time).total_seconds()
            
            # 检查结果
            if result.returncode == 0:
                print(f"\n✓ 配置 {mimo_config['name']} 优化成功!")
                print(f"  运行时间: {duration:.1f}秒 ({duration/60:.1f}分钟)")
                print(f"  输出配置: {output_config}")
                print(f"  输出头文件: {output_header}")
                
                # 读取优化结果
                if output_config.exists():
                    with open(output_config, 'r') as f:
                        result_data = json.load(f)
                    
                    return True, {
                        'mimo_config': mimo_config,
                        'duration': duration,
                        'output_config': str(output_config),
                        'output_header': str(output_header),
                        'baseline_ber': result_data.get('baseline_ber'),
                        'total_bitwidth_reduction': sum(
                            v['init_W'] - v['current_W'] 
                            for v in result_data.get('variables', {}).values()
                        )
                    }
                else:
                    return True, {
                        'mimo_config': mimo_config,
                        'duration': duration,
                        'output_config': str(output_config),
                        'output_header': str(output_header)
                    }
            else:
                print(f"\n✗ 配置 {mimo_config['name']} 优化失败!")
                print(f"  返回码: {result.returncode}")
                print(f"  输出:")
                print(result.stdout)
                return False, {
                    'mimo_config': mimo_config,
                    'error': result.stdout,
                    'returncode': result.returncode
                }
                
        except subprocess.TimeoutExpired:
            print(f"\n✗ 配置 {mimo_config['name']} 超时!")
            return False, {
                'mimo_config': mimo_config,
                'error': 'Timeout after 2 hours'
            }
        except Exception as e:
            print(f"\n✗ 配置 {mimo_config['name']} 发生错误: {e}")
            return False, {
                'mimo_config': mimo_config,
                'error': str(e)
            }
    
    def run_all_optimizations(self):
        """
        运行所有MIMO配置的位宽优化
        """
        configs = self.get_mimo_configs()
        
        print("\n" + "="*80)
        print("批量MIMO位宽优化")
        print("="*80)
        print(f"共 {len(configs)} 个配置需要优化")
        print(f"输出目录: {self.config['output_dir']}")
        print(f"BER阈值: {self.config['ber_threshold']}")
        print(f"优化顺序: {self.config['optimization_order']}")
        print("="*80)
        
        # 逐个运行优化
        for i, mimo_config in enumerate(configs, 1):
            print(f"\n进度: [{i}/{len(configs)}]")
            
            success, result_info = self.run_single_optimization(mimo_config)
            
            if success:
                self.results[mimo_config['name']] = result_info
            else:
                self.failed_configs.append(mimo_config['name'])
                self.results[mimo_config['name']] = result_info
        
        # 保存总结
        self.save_summary()
        
        # 打印最终报告
        self.print_final_report()
    
    def save_summary(self):
        """保存优化总结到JSON文件"""
        output_dir = Path(self.config['output_dir'])
        summary_file = output_dir / 'batch_optimization_summary.json'
        
        summary = {
            'timestamp': datetime.now().isoformat(),
            'config': self.config,
            'total_configs': len(self.results),
            'successful_configs': len(self.results) - len(self.failed_configs),
            'failed_configs': self.failed_configs,
            'results': self.results
        }
        
        with open(summary_file, 'w', encoding='utf-8') as f:
            json.dump(summary, f, indent=2, ensure_ascii=False)
        
        print(f"\n总结已保存到: {summary_file}")
    
    def print_final_report(self):
        """打印最终优化报告"""
        print("\n" + "="*80)
        print("批量优化最终报告")
        print("="*80)
        
        successful = len(self.results) - len(self.failed_configs)
        total = len(self.results)
        
        print(f"\n总配置数: {total}")
        print(f"成功: {successful}")
        print(f"失败: {len(self.failed_configs)}")
        
        if self.failed_configs:
            print(f"\n失败的配置:")
            for name in self.failed_configs:
                print(f"  - {name}")
        
        print(f"\n成功的配置:")
        for name, result in self.results.items():
            if name not in self.failed_configs:
                mimo = result['mimo_config']
                duration_min = result.get('duration', 0) / 60
                ber = result.get('baseline_ber', 'N/A')
                reduction = result.get('total_bitwidth_reduction', 'N/A')
                
                print(f"  ✓ {name}")
                print(f"      天线: {mimo['nt']}x{mimo['nr']}, 调制: {mimo['modulation']}QAM")
                print(f"      时间: {duration_min:.1f}分钟")
                print(f"      基准BER: {ber}")
                print(f"      位宽减少: {reduction} bits")
        
        print("\n" + "="*80)
        print("批量优化完成!")
        print("="*80)


def main():
    parser = argparse.ArgumentParser(
        description='批量MIMO位宽优化脚本',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  python3 batch_bitwidth_optimization.py \\
      --header /home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai/MyComplex_1.h \\
      --template /home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai/templates/sensitivity_types.hpp.jinja2 \\
      --output-dir /home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai/PYTHON_COPILOT/bitwidth_result

支持的MIMO配置:
  - 4x4, 16QAM
  - 8x8, 4QAM / 16QAM / 64QAM
  - 16x16, 16QAM / 64QAM
  - 32x32, 16QAM
  - 64x64, 16QAM
        """
    )
    
    parser.add_argument('--header', required=True,
                        help='MyComplex_1.h头文件路径')
    parser.add_argument('--template', required=True,
                        help='jinja2模板文件路径')
    parser.add_argument('--output-dir', required=True,
                        help='输出目录（所有优化结果将保存在此）')
    parser.add_argument('--data-path', default='/home/ggg_wufuqi/hls/MHGD/MHGD',
                        help='HLS测试数据基础路径 (默认: /home/ggg_wufuqi/hls/MHGD/MHGD)')
    parser.add_argument('--gaussian-noise-path',
                        default='/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai',
                        help='高斯噪声文件路径 (默认: /home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai)')
    parser.add_argument('--ber-threshold', type=float, default=1e-2,
                        help='BER阈值 (默认: 1e-2)')
    parser.add_argument('--order', dest='optimization_order', default='high_to_low',
                        choices=['sequential', 'high_to_low', 'low_to_high'],
                        help='优化顺序 (默认: high_to_low)')
    parser.add_argument('--initial-W', type=int, default=40,
                        help='初始总位宽 (默认: 40, 即整数8位+小数32位)')
    parser.add_argument('--initial-I', type=int, default=8,
                        help='初始整数位宽 (默认: 8)')
    
    args = parser.parse_args()
    
    # 构建配置
    config = {
        'header_file': args.header,
        'template_file': args.template,
        'output_dir': args.output_dir,
        'data_path': args.data_path,
        'gaussian_noise_path': args.gaussian_noise_path,
        'ber_threshold': args.ber_threshold,
        'optimization_order': args.optimization_order,
        'initial_W': args.initial_W,
        'initial_I': args.initial_I
    }
    
    # 验证文件存在
    if not os.path.exists(config['header_file']):
        print(f"错误: 头文件不存在: {config['header_file']}")
        sys.exit(1)
    
    if not os.path.exists(config['template_file']):
        print(f"错误: 模板文件不存在: {config['template_file']}")
        sys.exit(1)
    
    # 创建优化器并运行
    optimizer = BatchBitwidthOptimizer(config)
    optimizer.run_all_optimizations()


if __name__ == '__main__':
    main()
