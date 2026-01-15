# Multi_Configuration - JSON转头文件工具

## 概述

该工具用于将位宽优化结果JSON文件转换为C++头文件，支持单个文件转换和批量转换。

## 功能特性

- ✅ 从JSON配置文件读取优化结果
- ✅ 生成对应的C++头文件（包含所有typedef定义）
- ✅ 支持单个JSON转换
- ✅ 支持批量转换整个目录
- ✅ 自动提取MIMO配置信息
- ✅ 验证JSON格式完整性
- ✅ 自动生成文件名

## 使用方法

### 1. 单个JSON文件转换（自动命名）

```bash
cd Multi_Configuration
python3 json_to_header.py --json ../bitwidth_result/bitwidth_config_4_4_16QAM_SNR25.json
```

输出: `MyComplex_optimized_4_4_16QAM_SNR25.h`

### 2. 单个JSON文件转换（指定输出文件名）

```bash
cd Multi_Configuration
python3 json_to_header.py \
    --json ../bitwidth_result/bitwidth_config_4_4_16QAM_SNR25.json \
    --output MyComplex_custom_name.h
```

### 3. 批量转换

```bash
cd Multi_Configuration
python3 json_to_header.py \
    --batch ../bitwidth_result/ \
    --output-dir ./headers/
```

这将转换 `../bitwidth_result/` 目录下所有的 `bitwidth_config_*.json` 文件，并将结果保存到 `./headers/` 目录。

### 4. 查看帮助

```bash
python3 json_to_header.py --help
```

## 生成的头文件格式

生成的头文件包含：

```cpp
#pragma once
#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_math.h"
#include "hls_stream.h"

// 自动生成的优化位宽定义
// 源文件: bitwidth_config_4_4_16QAM_SNR25.json
// MIMO配置: 4x4 16QAM, SNR=25dB
// 生成时间: 2026-01-15 10:30:00

// 变量: H_hermitian (W=38, I=8, 减少2 bits)
typedef ap_fixed<38, 8> H_hermitian_t;

// 变量: y_received (W=36, I=8, 减少4 bits)
typedef ap_fixed<36, 8> y_received_t;

...
```

## JSON文件格式要求

JSON文件应包含以下结构：

```json
{
  "nt": 4,
  "nr": 4,
  "modulation": 16,
  "snr": 25,
  "variables": {
    "H_hermitian": {
      "init_W": 40,
      "init_I": 8,
      "optimal_W": 38,
      "optimal_I": 8
    },
    ...
  }
}
```

## HLS Math函数位宽冲突解决方案

如果在使用优化后的头文件时遇到 `hls_math.h` 函数的位宽冲突错误：

```
error: no matching function for call to 'generic_divide'
candidate template ignored: deduced conflicting values for parameter '_AP_W2'
```

### 解决方案1：使用标准除法运算符（推荐）

```cpp
// 原始代码
lr = hls_internal::generic_divide((local_temp_1_t)local_temp_1, (local_temp_2_t)local_temp_2);

// 修改为
lr = local_temp_1 / local_temp_2;  // HLS会自动处理类型转换
```

### 解决方案2：显式类型转换到统一计算类型

```cpp
// 定义统一的计算类型
typedef ap_fixed<40, 8> compute_t;

// 使用统一类型
lr = hls_internal::generic_divide((compute_t)local_temp_1, (compute_t)local_temp_2);
```

### 解决方案3：确保相关变量位宽一致

```cpp
// 在typedef定义时确保参与同一运算的变量使用相同位宽
typedef ap_fixed<W1, I1> local_temp_1_t;
typedef ap_fixed<W1, I1> local_temp_2_t;  // 使用相同的W和I
```

## 目录结构

```
Multi_Configuration/
├── json_to_header.py          # JSON转头文件工具脚本
├── README.md                   # 本文档
└── headers/                    # 批量转换输出目录（自动创建）
    ├── MyComplex_optimized_4_4_16QAM_SNR25.h
    ├── MyComplex_optimized_8_8_16QAM_SNR25.h
    └── ...
```

## 注意事项

1. **位宽选择优先级**：工具优先使用 `optimal_W/optimal_I`，如果不存在则使用 `init_W/init_I`
2. **文件覆盖**：如果输出文件已存在，将被覆盖
3. **JSON验证**：工具会验证JSON格式，缺少必要字段会报错
4. **权限问题**：确保对输出目录有写权限

## 常见问题

### Q1: 为什么生成的头文件位宽与JSON中的optimal_W不同？

A: 检查JSON文件是否包含 `optimal_W` 字段。如果缺少，工具会使用 `init_W` 作为默认值。

### Q2: 批量转换时部分文件失败怎么办？

A: 工具会继续处理其他文件，并在最后显示成功/失败统计。检查失败文件的JSON格式是否正确。

### Q3: 如何修改生成的头文件格式？

A: 编辑 `json_to_header.py` 中的 `generate_header_content()` 函数。

## 与bitwidth_optimization_v2.py的关系

- `bitwidth_optimization_v2.py`：执行位宽优化，生成JSON结果和初始头文件
- `json_to_header.py`（本工具）：从JSON结果重新生成头文件，确保使用最优位宽

建议工作流程：
1. 运行 `bitwidth_optimization_v2.py` 进行优化
2. 检查生成的JSON文件中的优化结果
3. 使用本工具从JSON重新生成头文件（确保使用最优位宽）

## 技术支持

如有问题，请检查：
- JSON文件格式是否正确
- Python版本是否 >= 3.6
- 输出目录是否有写权限
- JSON文件中是否包含 `variables` 字段
