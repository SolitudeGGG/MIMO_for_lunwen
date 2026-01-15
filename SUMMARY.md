# 项目总结和使用指南

## 已完成的工作

### 1. 核心脚本

已创建以下Python脚本实现完整的MIMO位宽优化流程：

| 脚本名称 | 功能描述 | 状态 |
|---------|---------|------|
| `generate_mimo_data.py` | 使用commpy生成MIMO测试数据 | ✅ 完成 |
| `bitwidth_optimization.py` | 单配置位宽优化引擎 | ✅ 完成 |
| `batch_optimization.py` | 批量优化多个配置 | ✅ 完成 |
| `subfunction_analyzer.py` | 分析子函数变量 | ✅ 完成 |
| `master_optimization.py` | 主控脚本整合所有流程 | ✅ 完成 |
| `test_system.py` | 系统功能测试 | ✅ 完成 |

### 2. 文档

| 文档名称 | 内容 | 状态 |
|---------|------|------|
| `README_OPTIMIZATION.md` | 完整的使用文档和API说明 | ✅ 完成 |
| `questions.md` | 问题列表和需要确认的事项 | ✅ 完成 |
| `SUMMARY.md` | 本文件，项目总结 | ✅ 完成 |

### 3. 配置文件

- `requirements.txt`: Python依赖管理
- `.gitignore`: 排除构建产物和生成文件
- `sensitivity_types.hpp.jinja2`: Jinja2模板（已存在）

## 功能特性

### 数据生成 (generate_mimo_data.py)

**支持的配置：**
- 天线配置: 8x8 (可扩展)
- 调制方式: 4QAM, 16QAM, 64QAM
- SNR范围: 5-25 dB (步长5dB)
- 样本数量: 可配置，默认10个

**生成的文件：**
```
mimo_data/
├── H_8x8_{modulation}_SNR{snr}.txt       # 信道矩阵
├── y_8x8_{modulation}_SNR{snr}.txt       # 接收信号
├── bits_8x8_{modulation}_SNR{snr}.txt    # 参考比特流
└── gaussian_noise/
    ├── gaussian_random_values_plus_1.txt  # 采样器1的噪声
    ├── gaussian_random_values_plus_2.txt  # 采样器2的噪声
    ├── gaussian_random_values_plus_3.txt  # 采样器3的噪声
    └── gaussian_random_values_plus_4.txt  # 采样器4的噪声
```

### 位宽优化 (bitwidth_optimization.py)

**优化算法：**
1. 解析MyComplex_1.h提取45个变量定义
2. 运行64位全精度基准测试获取BER
3. 对每个变量从大位宽到小位宽扫描
4. 每次减少2位，测试BER性能
5. 当BER超过阈值时停止优化该变量
6. 生成优化后的JSON配置和头文件

**可配置参数：**
- `--ber-threshold`: BER差异阈值（默认1e-4）
- `--order`: 优化顺序（sequential/high_to_low/low_to_high）
- `--nt`, `--nr`: 天线配置
- `--modulation`: 调制阶数
- `--snr`: 信噪比

### 批量优化 (batch_optimization.py)

自动为所有配置组合生成优化头文件：
- 天线: 8x8
- 调制: 4QAM, 16QAM, 64QAM
- SNR: 5, 10, 15, 20, 25 dB
- **总计**: 15个配置

### 子函数分析 (subfunction_analyzer.py)

**分析内容：**
- 识别MHGD_accel_hw.cpp中的~77个函数
- 提取局部变量声明
- 基于变量名智能建议位宽

**位宽建议规则：**
- `temp*`: W=32, I=12 (临时计算)
- `*norm*`: W=24, I=10 (范数)
- `grad*`: W=28, I=8 (梯度)
- `lr`, `step*`: W=18, I=4 (学习率/步长)
- `alpha*`: W=16, I=2 (系数)
- 默认: W=24, I=8

## 系统测试结果

运行 `python3 test_system.py` 的结果：

```
✓ 文件结构检查 - 所有文件存在
✓ 头文件解析 - 成功解析45个变量
✓ Jinja2模板 - 模板渲染正常
✓ C++文件解析 - 成功识别77个函数
✗ Python依赖 - numpy和commpy需要安装
```

## 使用流程

### 方案1: 完整自动化流程

```bash
# 1. 安装依赖
pip install -r requirements.txt

# 2. 运行完整流程
python3 master_optimization.py
```

这将自动执行：
1. 生成所有配置的MIMO测试数据
2. 运行基准测试
3. 分析子函数变量
4. 执行位宽优化
5. 生成总结报告

### 方案2: 分步执行

```bash
# 步骤1: 生成测试数据
python3 generate_mimo_data.py

# 步骤2: 分析子函数（可选）
python3 subfunction_analyzer.py

# 步骤3: 单配置优化（用于测试）
python3 bitwidth_optimization.py --nt 8 --nr 8 --modulation 16 --snr 20

# 步骤4: 批量优化所有配置
python3 batch_optimization.py
```

### 方案3: 仅运行特定步骤

```bash
# 只生成数据
python3 master_optimization.py --steps 1

# 跳过数据生成，只做优化
python3 master_optimization.py --skip-data-gen --steps 4
```

## 输出文件

### 数据文件
```
mimo_data/
├── H_*.txt               # 信道矩阵
├── y_*.txt               # 接收信号
├── bits_*.txt            # 参考比特流
└── gaussian_noise/       # 高斯噪声
```

### 优化结果
```
optimized_headers/
├── bitwidth_config_*.json    # 优化配置（含BER数据）
└── MyComplex_optimized_*.h   # 优化后的头文件
```

### 分析报告
```
subfunction_analysis_report.json        # 子函数分析
sensitivity_types_extended.hpp.jinja2  # 扩展模板
```

## 关键问题（需要用户确认）

详见 `questions.md`，主要包括：

1. **HLS编译环境**
   - 当前使用g++模拟编译
   - 实际应使用Vitis HLS的Csim
   - 需要修改`bitwidth_optimization.py`中的`compile_and_test`方法

2. **文件路径**
   - `main_hw.cpp`中硬编码了测试文件路径
   - 需要修改为与数据生成脚本的输出路径一致

3. **BER阈值**
   - 当前默认1e-4
   - 可能需要根据应用场景调整

4. **位宽优化策略**
   - 当前是逐变量独立优化
   - 可能需要考虑变量间的相互影响

5. **多配置头文件使用**
   - 如何在HLS项目中选择不同的优化头文件
   - 是否需要条件编译机制

## 技术实现细节

### 变量提取

使用正则表达式从MyComplex_1.h提取变量定义：
```python
pattern = r'typedef\s+ap_fixed<(\d+),\s*(\d+)(?:,\s*[A-Z_]+)?(?:,\s*[A-Z_]+)?>\s+(\w+)_t;'
```

成功提取45个变量：
- step_size, r_norm, lr, z_grad_real, z_prop_real, x_prop_real, H_real, y_real, v_real
- HH_H_real, grad_preconditioner_real, x_mmse_real, pmat_real, r_real, pr_prev_real
- temp_NtNt_real, temp_NtNr_real, temp_1_real, temp_Nt_real, temp_Nr_real
- sigma2eye_real, local_temp_1, local_temp_2
- 以及对应的虚部变量

### 模板渲染

使用Jinja2生成头文件：
```jinja2
{% for var_name, config in vars.items() %}
typedef ap_fixed<{{config.init_W}}, {{config.init_I}}> {{var_name}}_t;
{% endfor %}
```

### BER评估

从测试程序输出提取BER：
```python
ber_match = re.search(r'FINAL_BER:\s*([\d.e+-]+)', output)
```

## 扩展与定制

### 添加新的调制方式

1. 在`generate_mimo_data.py`中添加新的modulation_order
2. 在`MHGD_accel_hw.cpp`中添加对应的星座点和解调函数
3. 更新`MyComplex_1.h`中的`mu_1`参数

### 支持不同天线配置

1. 修改`MHGD_accel_hw.h`中的`Ntr_1`参数
2. 更新`batch_optimization.py`中的`antenna_configs`

### 自定义优化策略

修改`bitwidth_optimization.py`中的`optimize_single_variable`方法：
- 调整步长（当前为2）
- 修改停止条件
- 添加全局优化逻辑

## 性能预期

### 执行时间

- **数据生成**: 约1-2分钟（15个配置）
- **单配置优化**: 10-60分钟（取决于变量数量）
- **批量优化**: 2-15小时（15个配置 × 45个变量）

### 资源节省

基于位宽优化，预期可节省：
- **寄存器**: 20-40%
- **LUT**: 15-30%
- **DSP**: 取决于乘法器使用

## 下一步工作

1. **环境配置**
   - [ ] 安装Python依赖 (numpy, commpy)
   - [ ] 配置HLS工具链
   - [ ] 调整文件路径

2. **验证测试**
   - [ ] 运行数据生成脚本
   - [ ] 验证生成的数据格式
   - [ ] 测试单配置优化
   - [ ] 验证BER计算正确性

3. **实际优化**
   - [ ] 运行批量优化
   - [ ] 分析优化结果
   - [ ] 调整优化参数
   - [ ] 选择最优配置

4. **HLS综合**
   - [ ] 使用优化后的头文件进行HLS综合
   - [ ] 提取资源使用报告
   - [ ] 验证时序满足要求
   - [ ] 生成比较报告

## 技术支持

如有问题，请：
1. 查看 `questions.md` 中的常见问题
2. 查看 `README_OPTIMIZATION.md` 中的详细文档
3. 运行 `python3 test_system.py` 进行系统诊断

## 许可和贡献

欢迎提交Issue和Pull Request！

---

**创建日期**: 2026-01-07
**版本**: 1.0
**状态**: ✅ 基础实现完成，待实际环境验证
