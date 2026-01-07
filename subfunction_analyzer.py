#!/usr/bin/env python3
"""
子函数变量位宽优化分析脚本
分析MHGD_accel_hw.cpp中子函数的局部变量，生成位宽优化配置
"""

import re
import os
import json
from collections import defaultdict

class SubfunctionAnalyzer:
    def __init__(self, cpp_file):
        """
        初始化子函数分析器
        
        参数:
            cpp_file: MHGD_accel_hw.cpp文件路径
        """
        self.cpp_file = cpp_file
        self.functions = {}
        self.local_variables = defaultdict(list)
        
    def parse_cpp_file(self):
        """解析C++文件，提取函数和局部变量"""
        print(f"解析C++文件: {self.cpp_file}")
        
        with open(self.cpp_file, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        # 查找所有函数定义
        # 匹配函数签名: return_type function_name(params) {
        function_pattern = r'(\w+(?:<[^>]+>)?)\s+(\w+)\s*\([^)]*\)\s*\{'
        functions = re.finditer(function_pattern, content)
        
        for match in functions:
            return_type = match.group(1)
            func_name = match.group(2)
            
            # 跳过一些常见的非函数结构
            if func_name in ['if', 'for', 'while', 'switch', 'catch']:
                continue
            
            self.functions[func_name] = {
                'return_type': return_type,
                'local_vars': []
            }
        
        print(f"  找到 {len(self.functions)} 个函数")
        
        # 提取局部变量 (简化版本，仅识别明显的类型声明)
        self._extract_local_variables(content)
        
        return self.functions
    
    def _extract_local_variables(self, content):
        """提取局部变量声明"""
        
        # 常见的数值类型
        numeric_types = [
            r'int', r'float', r'double', r'long', r'short',
            r'like_float', r'Myreal', r'Myimage',
            r'ap_fixed<[^>]+>', r'ap_int<[^>]+>', r'ap_uint<[^>]+>',
            r'MyComplex', r'MyComplex_\w+',
            r'\w+_t'  # 自定义类型
        ]
        
        # 构建类型模式
        type_pattern = '|'.join(f'({t})' for t in numeric_types)
        
        # 匹配变量声明
        # 格式: type var_name = ...;  或  type var_name;
        var_pattern = rf'({type_pattern})\s+(\w+)(?:\s*=\s*[^;]+)?;'
        
        matches = re.finditer(var_pattern, content)
        
        seen_vars = set()
        for match in matches:
            var_type = match.group(1).strip()
            var_name = match.group(-1)  # 最后一个组是变量名
            
            # 避免重复
            var_key = f"{var_type}::{var_name}"
            if var_key in seen_vars:
                continue
            seen_vars.add(var_key)
            
            # 跳过一些明显的全局变量或数组声明
            if '[' in var_name or '(' in var_name:
                continue
            
            self.local_variables[var_type].append(var_name)
    
    def identify_non_templated_functions(self):
        """识别没有使用模板的函数"""
        print("\n分析非模板函数...")
        
        non_templated = []
        
        for func_name, func_info in self.functions.items():
            # 简化判断：如果函数名不包含常见的模板标识，认为是非模板
            if func_name not in ['c_matmultiple_hw', 'my_complex_add_hw', 
                                 'my_complex_copy_hw', 'my_complex_scal_hw',
                                 'complex_divide_hw', 'complex_multiply_hw',
                                 'complex_add_hw', 'complex_subtract_hw',
                                 'map_hw', 'Inverse_LU_hw', 'initMatrix_hw',
                                 'MulMatrix_hw', 'data_local', 'learning_rate_line_search_hw',
                                 'my_complex_sub_hw', 'c_eye_generate_hw', 'out_hw']:
                non_templated.append(func_name)
        
        print(f"  找到 {len(non_templated)} 个非模板函数")
        for func in non_templated[:10]:
            print(f"    - {func}")
        if len(non_templated) > 10:
            print(f"    ... 还有 {len(non_templated) - 10} 个")
        
        return non_templated
    
    def suggest_local_var_types(self):
        """建议局部变量的位宽类型"""
        print("\n建议局部变量位宽...")
        
        suggestions = {}
        
        # 基于变量名和类型推断合适的位宽
        for var_type, var_names in self.local_variables.items():
            for var_name in var_names:
                # 根据变量名模式推断位宽
                if 'temp' in var_name.lower():
                    # 临时变量可以使用较大位宽以避免溢出
                    suggestions[var_name] = {
                        'type': var_type,
                        'suggested_W': 32,
                        'suggested_I': 12,
                        'reason': '临时计算变量，需要足够精度'
                    }
                elif 'norm' in var_name.lower():
                    # 范数通常是非负值
                    suggestions[var_name] = {
                        'type': var_type,
                        'suggested_W': 24,
                        'suggested_I': 10,
                        'reason': '范数计算，需要正数表示'
                    }
                elif 'grad' in var_name.lower():
                    # 梯度
                    suggestions[var_name] = {
                        'type': var_type,
                        'suggested_W': 28,
                        'suggested_I': 8,
                        'reason': '梯度计算'
                    }
                elif 'lr' in var_name.lower() or 'step' in var_name.lower():
                    # 学习率或步长，通常是小数
                    suggestions[var_name] = {
                        'type': var_type,
                        'suggested_W': 18,
                        'suggested_I': 4,
                        'reason': '学习率/步长，小数值'
                    }
                elif 'alpha' in var_name.lower():
                    suggestions[var_name] = {
                        'type': var_type,
                        'suggested_W': 16,
                        'suggested_I': 2,
                        'reason': '系数，通常在0-1范围'
                    }
                else:
                    # 默认配置
                    suggestions[var_name] = {
                        'type': var_type,
                        'suggested_W': 24,
                        'suggested_I': 8,
                        'reason': '默认配置'
                    }
        
        # 打印部分建议
        print(f"  生成了 {len(suggestions)} 个变量的位宽建议")
        for var_name in list(suggestions.keys())[:5]:
            sug = suggestions[var_name]
            print(f"    {var_name}: W={sug['suggested_W']}, I={sug['suggested_I']} ({sug['reason']})")
        
        return suggestions
    
    def generate_extended_header_template(self, output_file, suggestions):
        """生成扩展的头文件模板，包含子函数变量"""
        print(f"\n生成扩展头文件: {output_file}")
        
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write("// 扩展的灵敏度分析类型定义 - 包含子函数局部变量\n")
            f.write("#pragma once\n")
            f.write("#include <ap_fixed.h>\n\n")
            
            f.write("// ====== 主要变量 (来自MyComplex_1.h) ======\n")
            f.write("{% for var_name, config in main_vars.items() %}\n")
            f.write("// 变量: {{ var_name }}\n")
            f.write("typedef ap_fixed<{{config.init_W}}, {{config.init_I}}> {{var_name}}_t;\n")
            f.write("{% endfor %}\n\n")
            
            f.write("// ====== 子函数局部变量 ======\n")
            for var_name, config in suggestions.items():
                f.write(f"// {var_name}: {config['reason']}\n")
                f.write(f"typedef ap_fixed<{config['suggested_W']}, {config['suggested_I']}> {var_name}_local_t;\n")
            
            f.write("\n// ====== 复数结构体 ======\n")
            f.write("// (保持与原始头文件一致)\n")
        
        print(f"  模板已生成: {output_file}")
    
    def save_analysis_report(self, output_file, suggestions):
        """保存分析报告到JSON文件"""
        report = {
            'functions': self.functions,
            'local_variables': dict(self.local_variables),
            'suggestions': suggestions,
            'non_templated_functions': self.identify_non_templated_functions()
        }
        
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(report, f, indent=2, ensure_ascii=False)
        
        print(f"\n分析报告已保存: {output_file}")

def main():
    """主函数"""
    import argparse
    
    parser = argparse.ArgumentParser(description='子函数变量位宽分析工具')
    parser.add_argument('--cpp', default='MHGD_accel_hw.cpp',
                       help='C++源文件路径')
    parser.add_argument('--output-template', default='sensitivity_types_extended.hpp.jinja2',
                       help='输出扩展模板文件')
    parser.add_argument('--output-report', default='subfunction_analysis_report.json',
                       help='输出分析报告JSON文件')
    
    args = parser.parse_args()
    
    # 创建分析器
    analyzer = SubfunctionAnalyzer(args.cpp)
    
    # 解析文件
    analyzer.parse_cpp_file()
    
    # 识别非模板函数
    analyzer.identify_non_templated_functions()
    
    # 生成建议
    suggestions = analyzer.suggest_local_var_types()
    
    # 生成扩展模板
    analyzer.generate_extended_header_template(args.output_template, suggestions)
    
    # 保存报告
    analyzer.save_analysis_report(args.output_report, suggestions)
    
    print("\n" + "=" * 60)
    print("子函数分析完成！")
    print("=" * 60)
    print(f"扩展模板: {args.output_template}")
    print(f"分析报告: {args.output_report}")

if __name__ == "__main__":
    main()
