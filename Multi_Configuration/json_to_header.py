#!/usr/bin/env python3
"""
JSON to Header File Converter
Converts bitwidth optimization JSON results to C++ header files
"""

import json
import argparse
import os
import sys
from pathlib import Path
from datetime import datetime

def load_json_config(json_path):
    """Load and validate JSON configuration file"""
    try:
        with open(json_path, 'r') as f:
            config = json.load(f)
        
        # Validate required fields
        if 'variables' not in config:
            raise ValueError("JSON missing 'variables' field")
        
        return config
    except json.JSONDecodeError as e:
        print(f"❌ Error parsing JSON: {e}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"❌ Error loading JSON: {e}", file=sys.stderr)
        sys.exit(1)

def extract_mimo_config(config):
    """Extract MIMO configuration from JSON"""
    mimo_info = {
        'nt': config.get('nt', 'Unknown'),
        'nr': config.get('nr', 'Unknown'),
        'modulation': config.get('modulation', 'Unknown'),
        'snr': config.get('snr', 'Unknown')
    }
    return mimo_info

def generate_header_content(config, json_filename):
    """Generate C++ header file content from JSON config"""
    mimo_info = extract_mimo_config(config)
    
    header = []
    header.append("#pragma once")
    header.append("#include \"ap_fixed.h\"")
    header.append("#include \"ap_int.h\"")
    header.append("#include \"hls_math.h\"")
    header.append("#include \"hls_stream.h\"")
    header.append("")
    header.append("// 自动生成的优化位宽定义")
    header.append(f"// 源文件: {json_filename}")
    header.append(f"// MIMO配置: {mimo_info['nt']}x{mimo_info['nr']} {mimo_info['modulation']}QAM, SNR={mimo_info['snr']}dB")
    header.append(f"// 生成时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    header.append("")
    
    variables = config.get('variables', {})
    
    for var_name, var_config in variables.items():
        # Get optimal bitwidth (prefer optimal, fallback to init)
        W = var_config.get('optimal_W', var_config.get('init_W', 40))
        I = var_config.get('optimal_I', var_config.get('init_I', 8))
        
        # Calculate bitwidth reduction
        init_W = var_config.get('init_W', 40)
        reduction = init_W - W
        
        # Add comment with optimization info
        header.append(f"// 变量: {var_name} (W={W}, I={I}, 减少{reduction} bits)")
        header.append(f"typedef ap_fixed<{W}, {I}> {var_name}_t;")
        header.append("")
    
    return '\n'.join(header)

def convert_json_to_header(json_path, output_path=None):
    """Convert single JSON file to header file"""
    json_path = Path(json_path)
    
    if not json_path.exists():
        print(f"❌ JSON file not found: {json_path}", file=sys.stderr)
        return False
    
    # Load JSON config
    config = load_json_config(json_path)
    
    # Generate output filename if not specified
    if output_path is None:
        # Extract config name from JSON filename
        # e.g., bitwidth_config_4_4_16QAM_SNR25.json -> MyComplex_optimized_4_4_16QAM_SNR25.h
        json_name = json_path.stem
        if json_name.startswith('bitwidth_config_'):
            config_name = json_name.replace('bitwidth_config_', '')
            output_path = f"MyComplex_optimized_{config_name}.h"
        else:
            output_path = json_name.replace('.json', '') + '_optimized.h'
    
    output_path = Path(output_path)
    
    # Generate header content
    header_content = generate_header_content(config, json_path.name)
    
    # Write to file
    try:
        output_path.parent.mkdir(parents=True, exist_ok=True)
        with open(output_path, 'w') as f:
            f.write(header_content)
        
        print(f"✓ 成功生成头文件: {output_path}")
        print(f"  变量数: {len(config.get('variables', {}))}")
        return True
    except Exception as e:
        print(f"❌ 写入头文件失败: {e}", file=sys.stderr)
        return False

def batch_convert(json_dir, output_dir):
    """Batch convert all JSON files in directory"""
    json_dir = Path(json_dir)
    output_dir = Path(output_dir)
    
    if not json_dir.exists():
        print(f"❌ 目录不存在: {json_dir}", file=sys.stderr)
        return
    
    # Find all JSON files
    json_files = list(json_dir.glob('bitwidth_config_*.json'))
    
    if not json_files:
        print(f"⚠️  在 {json_dir} 中未找到JSON文件", file=sys.stderr)
        return
    
    print(f"找到 {len(json_files)} 个JSON文件")
    print()
    
    output_dir.mkdir(parents=True, exist_ok=True)
    
    success_count = 0
    for json_file in json_files:
        # Generate output path
        config_name = json_file.stem.replace('bitwidth_config_', '')
        output_path = output_dir / f"MyComplex_optimized_{config_name}.h"
        
        if convert_json_to_header(json_file, output_path):
            success_count += 1
        print()
    
    print(f"批量转换完成: {success_count}/{len(json_files)} 成功")

def main():
    parser = argparse.ArgumentParser(
        description='将位宽优化JSON文件转换为C++头文件',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
使用示例:
  # 单个文件转换（自动命名）
  python3 json_to_header.py --json ../bitwidth_result/bitwidth_config_4_4_16QAM_SNR25.json
  
  # 单个文件转换（指定输出文件名）
  python3 json_to_header.py --json config.json --output MyComplex_opt.h
  
  # 批量转换
  python3 json_to_header.py --batch ../bitwidth_result/ --output-dir ./headers/
        '''
    )
    
    parser.add_argument('--json', type=str,
                       help='单个JSON文件路径')
    parser.add_argument('--output', type=str,
                       help='输出头文件路径（可选，默认自动生成）')
    parser.add_argument('--batch', type=str,
                       help='批量转换：JSON文件目录')
    parser.add_argument('--output-dir', type=str, default='./headers',
                       help='批量转换输出目录（默认: ./headers）')
    
    args = parser.parse_args()
    
    # Check arguments
    if args.json and args.batch:
        print("❌ 错误: 不能同时使用 --json 和 --batch", file=sys.stderr)
        sys.exit(1)
    
    if not args.json and not args.batch:
        parser.print_help()
        sys.exit(1)
    
    # Single file conversion
    if args.json:
        success = convert_json_to_header(args.json, args.output)
        sys.exit(0 if success else 1)
    
    # Batch conversion
    if args.batch:
        batch_convert(args.batch, args.output_dir)
        sys.exit(0)

if __name__ == '__main__':
    main()
