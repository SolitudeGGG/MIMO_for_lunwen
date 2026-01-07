# MIMO HLS 固定点位宽自动优化系统

本项目实现了一个完整的MIMO检测器HLS位宽自动优化系统，基于BER性能自动寻找最优固定点位宽配置。

## 功能特性

- ✅ 使用commpy库自动生成MIMO测试数据
- ✅ 支持多种调制方式 (4QAM, 16QAM, 64QAM)
- ✅ 支持多种SNR配置 (5-25 dB)
- ✅ 基于jinja2模板的头文件自动生成
- ✅ 自动化位宽优化算法
- ✅ 批量生成多种配置的优化头文件
- ✅ 子函数局部变量分析
- ✅ 完整的自动化流程

## 项目结构

```
MIMO_for_lunwen/
├── MHGD_accel_hw.cpp              # HLS加速器实现
├── MHGD_accel_hw.h                # 头文件
├── MyComplex_1.h                  # 固定点类型定义
├── main_hw.cpp                    # 测试主程序
├── sensitivity_types.hpp.jinja2   # Jinja2模板
├── 纯动态驱动的定点位宽优化流程.pdf  # 优化流程文档
│
├── generate_mimo_data.py          # MIMO数据生成脚本
├── bitwidth_optimization.py       # 位宽优化引擎
├── batch_optimization.py          # 批量优化脚本
├── subfunction_analyzer.py        # 子函数分析工具
├── master_optimization.py         # 主控脚本
├── questions.md                   # 问题与说明文档
└── README.md                      # 本文件
```

## 安装依赖

```bash
# 安装Python依赖
pip install numpy scikit-commpy jinja2

# 确保有C++编译器
# Ubuntu/Debian: sudo apt-get install g++
# 或者安装Xilinx Vitis HLS工具链
```

## 快速开始

### 方式1: 使用主控脚本（推荐）

运行完整的自动化流程：

```bash
python3 master_optimization.py
```

这将依次执行：
1. 生成MIMO测试数据
2. 运行基准测试（64位全精度）
3. 分析子函数变量
4. 执行位宽优化
5. 生成优化总结报告

### 方式2: 分步执行

#### 步骤1: 生成MIMO测试数据

```bash
python3 generate_mimo_data.py
```

这将生成：
- `mimo_data/H_*.txt`: 信道矩阵文件
- `mimo_data/y_*.txt`: 接收信号文件
- `mimo_data/bits_*.txt`: 参考比特流文件
- `mimo_data/gaussian_noise/`: 高斯噪声文件

#### 步骤2: 分析子函数变量

```bash
python3 subfunction_analyzer.py
```

输出：
- `sensitivity_types_extended.hpp.jinja2`: 扩展模板
- `subfunction_analysis_report.json`: 分析报告

#### 步骤3: 单配置位宽优化

```bash
python3 bitwidth_optimization.py \
    --nt 8 --nr 8 \
    --modulation 16 \
    --snr 20 \
    --ber-threshold 1e-4 \
    --order high_to_low
```

参数说明：
- `--nt`: 发送天线数
- `--nr`: 接收天线数
- `--modulation`: 调制阶数 (4, 16, 64)
- `--snr`: 信噪比(dB)
- `--ber-threshold`: BER差异阈值
- `--order`: 优化顺序 (sequential/high_to_low/low_to_high)

#### 步骤4: 批量优化多个配置

```bash
python3 batch_optimization.py
```

这将为所有配置组合生成优化的头文件：
- 天线配置: 8x8
- 调制方式: 4QAM, 16QAM, 64QAM
- SNR: 5, 10, 15, 20, 25 dB

## 输出文件

### 数据文件

生成在 `mimo_data/` 目录：

```
mimo_data/
├── H_8x8_4QAM_SNR5.txt
├── y_8x8_4QAM_SNR5.txt
├── bits_8x8_4QAM_SNR5.txt
├── ...
└── gaussian_noise/
    ├── gaussian_random_values_plus_1.txt
    ├── gaussian_random_values_plus_2.txt
    ├── gaussian_random_values_plus_3.txt
    └── gaussian_random_values_plus_4.txt
```

### 优化结果

生成在 `optimized_headers/` 目录：

```
optimized_headers/
├── bitwidth_config_8x8_4QAM_SNR5.json
├── MyComplex_optimized_8x8_4QAM_SNR5.h
├── bitwidth_config_8x8_16QAM_SNR20.json
├── MyComplex_optimized_8x8_16QAM_SNR20.h
└── ...
```

## 优化算法

### 基本流程

1. **解析现有配置**: 从 `MyComplex_1.h` 提取所有变量及其位宽
2. **获取基准BER**: 运行64位全精度配置，获得基准BER
3. **逐变量优化**: 
   - 对每个变量，从当前位宽开始
   - 逐步减小位宽（默认步长=2）
   - 编译并测试每个配置
   - 如果BER超过阈值，停止该变量的优化
4. **保存结果**: 生成优化后的配置文件和头文件

### 位宽优化策略

```
对于每个变量 var:
    current_W = var.init_W
    min_W = var.init_I + 2  # 至少保留整数位+2位小数
    
    for test_W in range(current_W, min_W, -step):
        生成测试配置(var, test_W)
        编译并运行测试
        计算BER
        
        if BER_diff <= threshold:
            继续减小位宽
        else:
            停止，使用上一个位宽
            break
```

### BER评估标准

- **可接受条件1**: `|BER_new - BER_baseline| <= threshold`
- **可接受条件2**: `BER_new <= BER_baseline * 1.1` (10%容忍度)

## 配置文件格式

### 位宽配置JSON

```json
{
  "mimo_config": {
    "Nt": 8,
    "Nr": 8,
    "modulation": 16,
    "SNR": 20
  },
  "baseline_ber": 0.00012345,
  "reference_ber_threshold": 0.0001,
  "variables": {
    "step_size": {
      "init_W": 18,
      "init_I": 4,
      "current_W": 14,
      "current_I": 4,
      "optimized": true
    },
    ...
  }
}
```

### 优化后的头文件

```cpp
#pragma once
#include <ap_fixed.h>

// 变量: step_size
typedef ap_fixed<14, 4> step_size_t;

// 变量: r_norm
typedef ap_fixed<16, 8> r_norm_t;

// ...
```

## 高级用法

### 自定义优化参数

```bash
python3 bitwidth_optimization.py \
    --nt 8 --nr 8 \
    --modulation 16 \
    --snr 20 \
    --ber-threshold 5e-5 \      # 更严格的阈值
    --order high_to_low         # 优先优化高位宽变量
```

### 只运行特定步骤

```bash
# 只生成数据
python3 master_optimization.py --steps 1

# 只做位宽优化
python3 master_optimization.py --steps 4 --skip-data-gen --skip-baseline

# 运行步骤1,3,4
python3 master_optimization.py --steps 1 3 4
```

### 批量优化时修改配置

编辑 `batch_optimization.py`:

```python
# 修改天线配置
antenna_configs = [(4, 4), (8, 8), (16, 16)]

# 修改调制方式
modulation_orders = [4, 16, 64]

# 修改SNR范围
snr_values = range(0, 30, 5)  # 0, 5, 10, ..., 25
```

## 子函数变量优化

### 识别的变量类型

脚本会自动识别以下类型的局部变量：
- `like_float`, `Myreal`, `Myimage`
- `ap_fixed<W,I>`, `ap_int<N>`, `ap_uint<N>`
- `MyComplex`, `MyComplex_*`
- 自定义 `*_t` 类型

### 位宽建议规则

基于变量名自动推断合适的位宽：
- `temp*` 变量: W=32, I=12 (临时计算，需要高精度)
- `*norm*` 变量: W=24, I=10 (范数，非负值)
- `grad*` 变量: W=28, I=8 (梯度)
- `lr`, `step*`: W=18, I=4 (学习率/步长，小数)
- `alpha*`: W=16, I=2 (系数，0-1范围)
- 默认: W=24, I=8

## 注意事项

### 编译环境

当前脚本假设使用g++编译。如果使用Vitis HLS，需要修改 `bitwidth_optimization.py` 中的 `compile_and_test` 方法。

### 测试数据路径

需要确保 `main_hw.cpp` 中的文件路径与生成的数据路径一致。可能需要修改：
- 第97-99行: bits/H/y文件路径
- 第172-179行: 高斯噪声文件路径

### 计算时间

- 单配置优化: 约10-60分钟（取决于变量数量）
- 批量优化: 可能需要数小时甚至更长

### 内存需求

- 8x8 MIMO: 约1-2GB
- 更大规模系统可能需要更多内存

## 故障排除

### 问题1: "ImportError: No module named 'commpy'"

**解决**: 
```bash
pip install scikit-commpy
```

### 问题2: "编译失败"

**检查**:
1. 确保安装了g++或HLS工具链
2. 检查头文件路径
3. 查看详细错误信息

### 问题3: "无法提取BER"

**原因**: 测试程序输出格式不匹配

**解决**: 
1. 确保 `main_hw.cpp` 中有 `FINAL_BER: <value>` 输出
2. 检查输出格式是否与正则表达式匹配

### 问题4: "优化结果BER过高"

**可能原因**:
1. BER阈值设置过严格
2. 某些变量的位宽已经最优
3. 变量之间存在相互影响

**建议**:
1. 增大BER阈值
2. 使用不同的优化顺序
3. 手动调整特定变量的位宽

## 扩展与定制

### 添加新的调制方式

1. 在 `generate_mimo_data.py` 中添加新的 `modulation_order`
2. 在 `MHGD_accel_hw.cpp` 中添加对应的星座点和解调函数
3. 更新 `MyComplex_1.h` 中的 `mu_1` 参数

### 支持不同天线配置

1. 修改 `MHGD_accel_hw.h` 中的 `Ntr_1` 参数
2. 更新 `batch_optimization.py` 中的 `antenna_configs`

### 自定义优化策略

在 `bitwidth_optimization.py` 中修改 `optimize_single_variable` 方法：
- 修改步长
- 修改停止条件
- 添加变量间依赖关系处理

## 参考文档

- 位宽优化流程: `纯动态驱动的定点位宽优化流程.pdf`
- 问题与说明: `questions.md`
- Jinja2模板文档: https://jinja.palletsprojects.com/

## 许可证

[请根据实际情况添加]

## 贡献

欢迎提交Issue和Pull Request！

## 联系方式

[请根据实际情况添加]
