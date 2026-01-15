#!/usr/bin/env python3
"""
Newpython批量MIMO位宽优化脚本（带编译验证）
自动对多种MIMO配置执行位宽优化，包含编译验证机制

支持的MIMO配置:
1. 4x4, 16QAM
2. 8x8, 4QAM
3. 8x8, 16QAM
4. 8x8, 64QAM
5. 16x16, 16QAM
6. 16x16, 64QAM
7. 32x32, 16QAM
8. 64x64, 16QAM

特性:
- 编译验证防止假性优化
- 保守优化策略
- 详细进度报告
- 错误日志收集
- 实时输出显示

作者: GitHub Copilot
日期: 2026-01-15
"""

import os
import sys
import json
import subprocess
import argparse
from datetime import datetime
from pathlib import Path
import time

class NewpythonBatchOptimizer:
    def __init__(self, script_path, output_dir='Newpython_result', ber_threshold=0.005):
        """
        初始化Newpython批量优化器
        
        参数:
            script_path: Newpython优化脚本的路径
            output_dir: 输出目录
            ber_threshold: BER阈值（默认0.005，比V2的0.01更严格）
        """
        self.script_path = script_path
        self.output_dir = Path(output_dir)
        self.ber_threshold = ber_threshold
        self.results = {}
        self.failed_configs = []
        self.start_time = None
        
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
    
    def run_single_optimization(self, mimo_config, index, total):
        """
        运行单个MIMO配置的位宽优化
        
        参数:
            mimo_config: MIMO配置字典
            index: 当前配置索引
            total: 总配置数
        
        返回:
            (success, result_info)
        """
        print("\n" + "="*80)
        print(f"进度: [{index}/{total}]")
        print(f"开始优化配置: {mimo_config['name']}")
        print(f"  天线数: {mimo_config['nt']}x{mimo_config['nr']}")
        print(f"  调制方式: {mimo_config['modulation']}QAM")
        print(f"  SNR: {mimo_config['snr']}dB")
        print(f"  BER阈值: {self.ber_threshold}")
        print("="*80)
        
        # 确保输出目录存在
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        # 构建命令
        cmd = [
            'python3', str(self.script_path),
            '--nt', str(mimo_config['nt']),
            '--nr', str(mimo_config['nr']),
            '--modulation', str(mimo_config['modulation']),
            '--snr', str(mimo_config['snr']),
            '--ber-threshold', str(self.ber_threshold),
        ]
        
        print(f"\n执行命令:")
        print(f"  {' '.join(cmd)}\n")
        
        # 记录开始时间
        config_start_time = time.time()
        
        try:
            # 运行优化脚本（实时输出）
            result = subprocess.run(
                cmd,
                cwd=os.getcwd(),  # 在当前工作目录运行
                check=False,
                text=True
            )
            
            # 计算耗时
            elapsed_time = time.time() - config_start_time
            
            if result.returncode == 0:
                print(f"\n✓ 配置 {mimo_config['name']} 优化成功")
                print(f"  耗时: {elapsed_time:.1f}秒 ({elapsed_time/60:.1f}分钟)")
                
                # 查找生成的结果文件
                config_file = self.output_dir / f"bitwidth_config_{mimo_config['name']}_SNR{mimo_config['snr']}.json"
                header_file = self.output_dir / f"MyComplex_optimized_{mimo_config['name']}_SNR{mimo_config['snr']}.h"
                
                result_info = {
                    'status': 'success',
                    'elapsed_time': elapsed_time,
                    'config_file': str(config_file) if config_file.exists() else None,
                    'header_file': str(header_file) if header_file.exists() else None,
                    'mimo_config': mimo_config
                }
                
                # 尝试读取优化结果统计
                if config_file.exists():
                    try:
                        with open(config_file, 'r') as f:
                            config_data = json.load(f)
                            if 'optimization_summary' in config_data:
                                result_info['summary'] = config_data['optimization_summary']
                    except Exception as e:
                        print(f"  警告: 无法读取配置文件: {e}")
                
                return True, result_info
            else:
                print(f"\n✗ 配置 {mimo_config['name']} 优化失败")
                print(f"  返回码: {result.returncode}")
                print(f"  耗时: {elapsed_time:.1f}秒")
                
                return False, {
                    'status': 'failed',
                    'returncode': result.returncode,
                    'elapsed_time': elapsed_time,
                    'mimo_config': mimo_config
                }
                
        except Exception as e:
            elapsed_time = time.time() - config_start_time
            print(f"\n✗ 配置 {mimo_config['name']} 执行异常")
            print(f"  错误: {str(e)}")
            print(f"  耗时: {elapsed_time:.1f}秒")
            
            return False, {
                'status': 'exception',
                'error': str(e),
                'elapsed_time': elapsed_time,
                'mimo_config': mimo_config
            }
    
    def run_batch(self, configs=None):
        """
        运行批量优化
        
        参数:
            configs: MIMO配置列表（如果为None，使用默认配置）
        
        返回:
            优化结果字典
        """
        if configs is None:
            configs = self.get_mimo_configs()
        
        total_configs = len(configs)
        self.start_time = time.time()
        
        print("\n" + "="*80)
        print("Newpython批量MIMO位宽优化")
        print("="*80)
        print(f"总配置数: {total_configs}")
        print(f"BER阈值: {self.ber_threshold}")
        print(f"输出目录: {self.output_dir}")
        print(f"优化脚本: {self.script_path}")
        print(f"开始时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print("="*80)
        
        # 逐个优化
        for i, config in enumerate(configs, 1):
            success, result_info = self.run_single_optimization(config, i, total_configs)
            
            if success:
                self.results[config['name']] = result_info
            else:
                self.failed_configs.append(config['name'])
                self.results[config['name']] = result_info
            
            # 显示当前进度
            success_count = len([r for r in self.results.values() if r['status'] == 'success'])
            failed_count = len(self.failed_configs)
            
            print(f"\n当前进度: {i}/{total_configs} 完成")
            print(f"  成功: {success_count}, 失败: {failed_count}")
            
            # 估算剩余时间
            if i < total_configs:
                elapsed = time.time() - self.start_time
                avg_time_per_config = elapsed / i
                remaining_configs = total_configs - i
                estimated_remaining = avg_time_per_config * remaining_configs
                
                print(f"  已用时间: {elapsed/60:.1f}分钟")
                print(f"  预计剩余: {estimated_remaining/60:.1f}分钟")
        
        # 生成最终报告
        self.generate_summary()
        
        return self.results
    
    def generate_summary(self):
        """
        生成批量优化总结报告
        """
        total_time = time.time() - self.start_time
        success_count = len([r for r in self.results.values() if r['status'] == 'success'])
        failed_count = len(self.failed_configs)
        total_count = len(self.results)
        
        print("\n" + "="*80)
        print("批量优化完成 - 总结报告")
        print("="*80)
        print(f"完成时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print(f"总耗时: {total_time/60:.1f}分钟 ({total_time/3600:.2f}小时)")
        print(f"\n配置统计:")
        print(f"  总配置数: {total_count}")
        print(f"  成功: {success_count}")
        print(f"  失败: {failed_count}")
        print(f"  成功率: {success_count/total_count*100:.1f}%")
        
        if success_count > 0:
            print(f"\n✓ 成功的配置:")
            for name, info in self.results.items():
                if info['status'] == 'success':
                    print(f"  - {name} ({info['elapsed_time']/60:.1f}分钟)")
                    if 'summary' in info:
                        summary = info['summary']
                        if 'total_bitwidth_reduction' in summary:
                            print(f"      位宽减少: {summary['total_bitwidth_reduction']} bits")
        
        if failed_count > 0:
            print(f"\n✗ 失败的配置:")
            for name, info in self.results.items():
                if info['status'] != 'success':
                    print(f"  - {name} ({info['elapsed_time']/60:.1f}分钟)")
                    if 'error' in info:
                        print(f"      错误: {info['error']}")
        
        print("\n输出文件:")
        print(f"  目录: {self.output_dir}/")
        print(f"  - bitwidth_config_*.json (配置文件)")
        print(f"  - MyComplex_optimized_*.h (优化头文件)")
        print(f"  - batch_optimization_summary.json (本报告)")
        
        # 保存JSON格式的总结
        summary_file = self.output_dir / 'batch_optimization_summary.json'
        summary_data = {
            'start_time': datetime.fromtimestamp(self.start_time).isoformat(),
            'end_time': datetime.now().isoformat(),
            'total_time_seconds': total_time,
            'total_configs': total_count,
            'successful_configs': success_count,
            'failed_configs': failed_count,
            'success_rate': success_count / total_count if total_count > 0 else 0,
            'ber_threshold': self.ber_threshold,
            'results': self.results,
            'system': 'Newpython (with compilation verification)'
        }
        
        try:
            with open(summary_file, 'w') as f:
                json.dump(summary_data, f, indent=2)
            print(f"\n总结报告已保存: {summary_file}")
        except Exception as e:
            print(f"\n警告: 无法保存总结报告: {e}")
        
        print("="*80 + "\n")

def main():
    """
    主函数
    """
    parser = argparse.ArgumentParser(
        description='Newpython批量MIMO位宽优化（带编译验证）',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
示例用法:
  # 使用默认配置优化所有MIMO配置
  python3 batch_bitwidth_optimization.py
  
  # 指定不同的BER阈值
  python3 batch_bitwidth_optimization.py --ber-threshold 0.003
  
  # 指定输出目录
  python3 batch_bitwidth_optimization.py --output-dir my_results
  
  # 只优化特定配置
  python3 batch_bitwidth_optimization.py --configs 4_4_16QAM 8_8_16QAM

注意:
  - 必须在HLS源文件目录（包含MyComplex_1.h的目录）运行
  - 确保已加载Vitis HLS环境
  - 批量优化可能需要数小时，建议使用nohup后台运行
        '''
    )
    
    parser.add_argument('--ber-threshold', type=float, default=0.005,
                        help='BER阈值（默认0.005，比V2更严格）')
    parser.add_argument('--output-dir', type=str, default='Newpython_result',
                        help='输出目录（默认Newpython_result）')
    parser.add_argument('--configs', nargs='+', type=str,
                        help='要优化的配置列表（如：4_4_16QAM 8_8_16QAM）')
    
    args = parser.parse_args()
    
    # 确定脚本路径
    script_dir = Path(__file__).parent
    script_path = script_dir / 'bitwidth_optimization.py'
    
    if not script_path.exists():
        print(f"错误: 找不到优化脚本: {script_path}")
        sys.exit(1)
    
    # 创建批量优化器
    optimizer = NewpythonBatchOptimizer(
        script_path=script_path,
        output_dir=args.output_dir,
        ber_threshold=args.ber_threshold
    )
    
    # 获取配置列表
    if args.configs:
        # 过滤用户指定的配置
        all_configs = optimizer.get_mimo_configs()
        configs = [c for c in all_configs if c['name'] in args.configs]
        
        if not configs:
            print(f"错误: 未找到指定的配置: {args.configs}")
            print(f"可用配置: {[c['name'] for c in all_configs]}")
            sys.exit(1)
    else:
        configs = None  # 使用所有默认配置
    
    # 运行批量优化
    try:
        results = optimizer.run_batch(configs)
        
        # 根据结果设置退出码
        failed_count = len(optimizer.failed_configs)
        sys.exit(0 if failed_count == 0 else 1)
        
    except KeyboardInterrupt:
        print("\n\n中断: 用户取消批量优化")
        sys.exit(130)
    except Exception as e:
        print(f"\n错误: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == '__main__':
    main()
