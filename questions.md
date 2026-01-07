# MIMO位宽优化项目 - 问题与说明

## 实现概述

本项目实现了一个自动化的MIMO HLS位宽优化系统，包含以下主要功能：

1. **MIMO数据生成** (`generate_mimo_data.py`)
   - 使用commpy库生成4QAM/16QAM/64QAM调制数据
   - 支持多种SNR配置(5-25dB)
   - 生成H矩阵、y向量和参考比特流

2. **位宽优化引擎** (`bitwidth_optimization.py`)
   - 自动解析MyComplex_1.h中的变量定义
   - 使用jinja2模板生成不同位宽配置的头文件
   - 基于BER性能自动寻找最优位宽

3. **批量优化** (`batch_optimization.py`)
   - 支持多种MIMO配置的批量优化
   - 为不同调制阶数和SNR生成优化头文件

4. **子函数分析** (`subfunction_analyzer.py`)
   - 分析MHGD_accel_hw.cpp中的非模板函数
   - 识别局部变量并提供位宽优化建议

5. **主控脚本** (`master_optimization.py`)
   - 整合所有步骤的自动化流程

## 需要用户确认的问题

### 1. HLS编译和测试环境

**问题**: 当前脚本中的编译和测试命令是基于假设的，需要根据实际环境调整。

**当前假设**:
```python
# 编译命令
compile_cmd = ["g++", "-std=c++11", "-I.", "-o", "test_mimo", 
              "main_hw.cpp", "MHGD_accel_hw.cpp", "-lm"]

# 运行测试
test_cmd = ["./test_mimo"]
```

**需要确认**:
- 是否使用Vitis HLS进行Csim？
- 如果是，正确的编译和运行命令是什么？
- 是否需要设置特定的环境变量或路径？
- 测试数据文件的路径是否正确？

**建议**: 在`bitwidth_optimization.py`中的`compile_and_test`方法需要根据实际的HLS工具链进行调整。

---

### 2. 测试数据路径

**问题**: `main_hw.cpp`中硬编码了多个文件路径，需要确认或修改。

**当前路径** (第97-99行):
```cpp
char bits_file[1024] = "/home/ggg_wufuqi/hls/MHGD/MHGD/8_8_16QAM/reference_file/bits_SNR=";
char H_file[1024] = "/home/ggg_wufuqi/hls/MHGD/MHGD/8_8_16QAM/input_file/H_SNR=";
char y_file[1024] = "/home/ggg_wufuqi/hls/MHGD/MHGD/8_8_16QAM/input_file/y_SNR=";
```

**需要确认**:
- 这些路径是否需要改为相对路径？
- 是否需要将数据生成脚本的输出路径与这些路径对齐？

**建议**: 修改`main_hw.cpp`使用相对路径，或者在数据生成脚本中生成到指定目录。

---

### 3. 高斯噪声文件路径

**问题**: `main_hw.cpp`第172-179行读取高斯噪声文件的路径是硬编码的。

**当前路径**:
```cpp
read_gaussian_data_hw("/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai/gaussian_random_values_plus.txt", ...);
```

**需要确认**:
- 是否需要使用数据生成脚本生成的噪声文件？
- 噪声文件格式是否与现有代码兼容？

---

### 4. BER阈值设定

**问题**: 位宽优化的BER阈值需要根据应用需求设定。

**当前默认值**: `1e-4`

**需要确认**:
- 这个阈值是否合适？
- 不同调制阶数和SNR是否需要不同的阈值？

---

### 5. 位宽优化策略

**问题**: 当前实现的优化策略是逐个变量独立优化。

**当前策略**:
- 从大位宽到小位宽逐步减少
- 每减少2位进行一次测试
- 如果BER超过阈值则停止该变量的优化

**需要确认**:
- 是否需要考虑变量之间的相互影响？
- 是否需要全局优化而非贪心优化？
- 步长（当前为2）是否合适？

---

### 6. 子函数变量识别

**问题**: `subfunction_analyzer.py`使用正则表达式识别变量，可能不够精确。

**局限性**:
- 无法识别复杂的变量声明
- 无法区分局部变量和全局变量
- 无法追踪变量的实际使用范围

**需要确认**:
- 当前的识别结果是否满足需求？
- 是否需要手动补充遗漏的变量？

---

### 7. 多配置头文件管理

**问题**: 为每个配置生成独立的头文件，如何在HLS项目中使用？

**当前方案**: 生成多个独立的头文件，如：
- `MyComplex_optimized_8x8_4QAM_SNR5.h`
- `MyComplex_optimized_8x8_16QAM_SNR20.h`
- ...

**需要确认**:
- 如何在编译时选择使用哪个头文件？
- 是否需要条件编译机制？
- 是否需要生成统一的头文件选择器？

---

### 8. 调制方式支持

**问题**: 当前`MHGD_accel_hw.h`定义的调制阶数为`mu_1 = 4`（16QAM）。

**需要确认**:
- 如何支持不同调制阶数的切换？
- 是否需要修改源代码以支持动态调制选择？
- 星座点数组是否需要根据调制阶数调整？

---

### 9. 采样器数量

**问题**: 当前代码支持4个并行采样器，但注释中有8个采样器的代码。

**需要确认**:
- 是否需要支持可配置的采样器数量？
- 不同的采样器数量是否影响BER性能？
- 优化后的位宽是否对采样器数量敏感？

---

### 10. 性能验证

**问题**: 如何验证优化后的位宽配置在实际HLS综合中的资源占用？

**需要**:
- 运行HLS综合获取资源报告
- 比较不同位宽配置的资源使用
- 验证时序是否满足要求

**建议**: 添加自动化脚本运行HLS综合并提取资源报告。

---

## 使用建议

### 快速开始

1. **安装依赖**:
```bash
pip install numpy scikit-commpy jinja2
```

2. **生成测试数据**:
```bash
python3 generate_mimo_data.py
```

3. **运行单配置优化** (调试用):
```bash
python3 bitwidth_optimization.py --nt 8 --nr 8 --modulation 16 --snr 20
```

4. **运行完整流程**:
```bash
python3 master_optimization.py
```

5. **批量优化多个配置**:
```bash
python3 batch_optimization.py
```

### 自定义配置

可以通过修改脚本中的参数来自定义优化流程：

- 调整SNR范围: 修改`generate_mimo_data.py`中的`snr_range`
- 调整位宽步长: 修改`bitwidth_optimization.py`中的`step`参数
- 调整BER阈值: 使用`--ber-threshold`参数
- 调整优化顺序: 使用`--order`参数 (sequential/high_to_low/low_to_high)

---

## 待完善的功能

1. **HLS TCL脚本生成**: 自动生成用于综合和实现的TCL脚本
2. **资源报告解析**: 自动解析HLS综合报告，提取资源使用信息
3. **帕累托前沿分析**: 生成BER vs 资源使用的权衡曲线
4. **可视化工具**: 生成位宽优化结果的可视化图表
5. **回归测试**: 自动化回归测试确保优化不破坏功能

---

## 已知限制

1. **编译环境依赖**: 需要配置正确的HLS编译环境
2. **计算时间**: 批量优化可能需要很长时间（取决于配置数量和测试样本数）
3. **内存消耗**: 大规模MIMO系统可能需要大量内存
4. **精度损失**: 过度优化可能导致无法接受的BER降级

---

## 技术细节

### 位宽表示

ap_fixed<W, I> 其中：
- W: 总位宽
- I: 整数位宽（包括符号位）
- 小数位宽 = W - I

### BER计算

BER = (错误比特数) / (总比特数)

基准BER: 使用64位全精度计算得到的BER
优化目标: BER增量 < threshold 或 BER < baseline_BER * 1.1

---

如有任何问题，请在此文件中记录，我将在后续工作中解答。
