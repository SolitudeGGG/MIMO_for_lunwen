#!/usr/bin/env python3
"""
测试脚本 - 验证各个模块的基本功能
"""

import os
import sys
import json

def test_imports():
    """测试Python依赖是否正确安装"""
    print("=" * 60)
    print("测试 1: Python依赖检查")
    print("=" * 60)
    
    required = {
        'numpy': 'numpy',
        'jinja2': 'jinja2', 
        'commpy': 'scikit-commpy'
    }
    
    all_ok = True
    for module, package in required.items():
        try:
            __import__(module)
            print(f"  ✓ {package}")
        except ImportError:
            print(f"  ✗ {package} - 未安装")
            all_ok = False
    
    return all_ok

def test_file_structure():
    """测试文件结构是否完整"""
    print("\n" + "=" * 60)
    print("测试 2: 文件结构检查")
    print("=" * 60)
    
    required_files = [
        'MyComplex_1.h',
        'MHGD_accel_hw.cpp',
        'MHGD_accel_hw.h',
        'main_hw.cpp',
        'sensitivity_types.hpp.jinja2',
        'generate_mimo_data.py',
        'bitwidth_optimization.py',
        'batch_optimization.py',
        'subfunction_analyzer.py',
        'master_optimization.py'
    ]
    
    all_ok = True
    for file in required_files:
        if os.path.exists(file):
            print(f"  ✓ {file}")
        else:
            print(f"  ✗ {file} - 不存在")
            all_ok = False
    
    return all_ok

def test_header_parsing():
    """测试头文件解析功能"""
    print("\n" + "=" * 60)
    print("测试 3: 头文件解析")
    print("=" * 60)
    
    try:
        import re
        
        with open('MyComplex_1.h', 'r', encoding='utf-8') as f:
            content = f.read()
        
        pattern = r'typedef\s+ap_fixed<(\d+),\s*(\d+)(?:,\s*[A-Z_]+)?(?:,\s*[A-Z_]+)?>\s+(\w+)_t;'
        matches = re.findall(pattern, content)
        
        print(f"  找到 {len(matches)} 个变量定义")
        
        # 显示前5个
        for i, (width, int_bits, var_name) in enumerate(matches[:5], 1):
            print(f"    {i}. {var_name}: W={width}, I={int_bits}")
        
        if len(matches) > 5:
            print(f"    ... 还有 {len(matches) - 5} 个")
        
        return len(matches) > 0
    except Exception as e:
        print(f"  ✗ 解析失败: {e}")
        return False

def test_template():
    """测试Jinja2模板"""
    print("\n" + "=" * 60)
    print("测试 4: Jinja2模板")
    print("=" * 60)
    
    try:
        from jinja2 import Template
        
        with open('sensitivity_types.hpp.jinja2', 'r', encoding='utf-8') as f:
            template_content = f.read()
        
        print(f"  模板文件大小: {len(template_content)} 字节")
        
        # 测试渲染
        template = Template(template_content)
        test_vars = {
            'test_var': {
                'init_W': 18,
                'init_I': 4
            }
        }
        
        rendered = template.render(vars=test_vars)
        
        if 'test_var' in rendered and 'ap_fixed<18, 4>' in rendered:
            print("  ✓ 模板渲染正常")
            return True
        else:
            print("  ✗ 模板渲染结果异常")
            return False
            
    except Exception as e:
        print(f"  ✗ 模板测试失败: {e}")
        return False

def test_data_generation_prep():
    """测试数据生成准备"""
    print("\n" + "=" * 60)
    print("测试 5: 数据生成准备")
    print("=" * 60)
    
    try:
        import numpy as np
        from commpy.modulation import QAMModem
        
        # 测试QAM调制器
        for m in [4, 16, 64]:
            modem = QAMModem(m)
            bits_per_symbol = {4: 2, 16: 4, 64: 6}[m]
            bits = np.random.randint(0, 2, bits_per_symbol * 8)
            symbols = modem.modulate(bits)
            print(f"  ✓ {m}-QAM 调制测试通过 ({len(symbols)} 符号)")
        
        # 测试信道矩阵生成
        H_real = np.random.randn(8, 8) / np.sqrt(2)
        H_imag = np.random.randn(8, 8) / np.sqrt(2)
        H = H_real + 1j * H_imag
        print(f"  ✓ 信道矩阵生成测试通过 (8x8)")
        
        return True
        
    except Exception as e:
        print(f"  ✗ 数据生成准备失败: {e}")
        return False

def test_cpp_parsing():
    """测试C++文件解析"""
    print("\n" + "=" * 60)
    print("测试 6: C++文件解析")
    print("=" * 60)
    
    try:
        import re
        
        with open('MHGD_accel_hw.cpp', 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        # 查找函数定义
        function_pattern = r'(\w+(?:<[^>]+>)?)\s+(\w+)\s*\([^)]*\)\s*\{'
        functions = re.findall(function_pattern, content)
        
        # 过滤掉一些非函数的匹配
        valid_functions = [f for _, f in functions if f not in ['if', 'for', 'while', 'switch', 'catch']]
        
        print(f"  找到约 {len(valid_functions)} 个函数")
        
        # 显示一些函数名
        for func in list(set(valid_functions))[:5]:
            print(f"    - {func}")
        
        return len(valid_functions) > 0
        
    except Exception as e:
        print(f"  ✗ C++解析失败: {e}")
        return False

def main():
    """运行所有测试"""
    print("\n" + "#" * 60)
    print("MIMO位宽优化系统 - 功能测试")
    print("#" * 60 + "\n")
    
    results = {
        'Python依赖': test_imports(),
        '文件结构': test_file_structure(),
        '头文件解析': test_header_parsing(),
        'Jinja2模板': test_template(),
        '数据生成准备': test_data_generation_prep(),
        'C++文件解析': test_cpp_parsing()
    }
    
    # 总结
    print("\n" + "=" * 60)
    print("测试总结")
    print("=" * 60)
    
    passed = sum(1 for v in results.values() if v)
    total = len(results)
    
    for test_name, result in results.items():
        status = "✓ 通过" if result else "✗ 失败"
        print(f"  {test_name}: {status}")
    
    print(f"\n总计: {passed}/{total} 测试通过")
    
    if passed == total:
        print("\n✓ 所有测试通过！系统已准备就绪。")
        return 0
    else:
        print("\n✗ 部分测试失败，请检查上面的错误信息。")
        return 1

if __name__ == "__main__":
    sys.exit(main())
