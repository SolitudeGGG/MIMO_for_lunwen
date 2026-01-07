# 位宽优化脚本使用指南

## 脚本执行后的预期结果

当你运行位宽优化脚本后，系统会产生以下输出：

### 1. 终端输出

#### 初始化阶段
```
解析头文件: MyComplex_1.h
找到45个变量定义
加载模板: sensitivity_types.hpp.jinja2

============================================================
开始位宽优化
============================================================

获取64位全精度基准BER...
  测试配置: baseline
    运行Vitis HLS Csim...
    BER = 0.00123456
基准BER: 0.00123456
```

#### 优化过程
```
优化顺序: high_to_low
变量数量: 45

[1/45] 
优化变量: MyComplex_H
  测试配置: MyComplex_H_W38
    运行Vitis HLS Csim...
    BER = 0.00123458
  W=38 通过 (BER=0.00123458, diff=0.00000002)
  测试配置: MyComplex_H_W36
    运行Vitis HLS Csim...
    BER = 0.00125678
  W=36 BER过高 (BER=0.00125678, diff=0.00002222)
  最优位宽: W=38, I=8

[2/45]
优化变量: MyComplex_y
  ...

[45/45]
优化变量: MyComplex_temp_1
  ...

============================================================
位宽优化完成
============================================================

配置已保存到: bitwidth_config_8x8_16QAM_SNR20.json
优化后的头文件已生成: MyComplex_optimized_8x8_16QAM_SNR20.h

位宽优化结果摘要:
---------------------------------------------------------
变量名                          初始位宽        当前位宽        减少量
---------------------------------------------------------
MyComplex_H                     40              38              2
MyComplex_y                     40              36              4
MyComplex_x_mmse                40              34              6
...
---------------------------------------------------------
总位宽减少: 156 bits

优化流程完成！
```

### 2. 生成的文件

#### A. JSON配置文件
**文件名**: `bitwidth_config_8x8_16QAM_SNR20.json`

**内容示例**:
```json
{
  "mimo_config": {
    "Nt": 8,
    "Nr": 8,
    "modulation": 16,
    "SNR": 20,
    "data_path": "/home/ggg_wufuqi/hls/MHGD/MHGD",
    "gaussian_noise_path": "/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai"
  },
  "baseline_ber": 0.00123456,
  "reference_ber_threshold": 0.0001,
  "variables": {
    "MyComplex_H": {
      "init_W": 40,
      "init_I": 8,
      "current_W": 38,
      "current_I": 8,
      "min_W": 16,
      "optimized": true
    },
    "MyComplex_y": {
      "init_W": 40,
      "init_I": 8,
      "current_W": 36,
      "current_I": 8,
      "min_W": 16,
      "optimized": true
    },
    ...
  }
}
```

**用途**:
- 记录优化过程和结果
- 可用于后续分析和比较
- 包含基准BER和所有变量的位宽配置

#### B. 优化后的头文件
**文件名**: `MyComplex_optimized_8x8_16QAM_SNR20.h`

**内容示例**:
```cpp
// 优化后的固定点类型定义
// MIMO配置: 8x8天线, 16QAM调制, SNR=20dB
// 基准BER: 0.00123456

typedef ap_fixed<38,8> MyComplex_H_t;     // 优化: 40 -> 38
typedef ap_fixed<36,8> MyComplex_y_t;     // 优化: 40 -> 36
typedef ap_fixed<34,8> MyComplex_x_mmse_t; // 优化: 40 -> 34
...
```

**用途**:
- 直接替换原始的MyComplex_1.h使用
- 包含所有变量的优化位宽定义
- 带有注释说明优化结果

#### C. HLS日志文件
**文件名**: `build_baseline.log`, `build_MyComplex_H_W38.log`, ...

**内容**:
- 完整的Vitis HLS Csim输出
- 编译信息
- 测试运行日志
- **BER结果** (关键信息)

**示例**:
```
INFO: [SIM 2] *************** CSIM start ***************
...
运行测试...
FINAL_BER: 0.00123456
...
INFO: [SIM 22] *************** CSIM finish ***************
```

**用途**:
- 调试和验证
- 检查编译错误
- 确认BER提取正确

#### D. TCL脚本文件
**文件名**: `run_hls_baseline.tcl`, `run_hls_MyComplex_H_W38.tcl`, ...

**内容** (自动生成，执行后会被清理):
```tcl
# Bitwidth Optimization HLS TCL Script
# MIMO Config: 8_8_16QAM

open_project -reset bitwidth_opt_project
add_files MHGD_accel_hw.cpp
add_files MHGD_accel_hw.h
add_files MyComplex_1.h
add_files -tb main_hw.cpp

# 测试数据文件
add_files -tb /home/ggg_wufuqi/hls/MHGD/MHGD/8_8_16QAM/reference_file/bits_SNR=20.txt -cflags "-I."
add_files -tb /home/ggg_wufuqi/hls/MHGD/MHGD/8_8_16QAM/input_file/H_SNR=20.txt -cflags "-I."
...

set_top MHGD_detect_accel_hw
open_solution -reset "solution1"
...
csim_design -ldflags "-lm"
exit
```

### 3. 文件夹结构

运行后的目录结构:
```
/home/runner/work/MIMO_for_lunwen/MIMO_for_lunwen/
├── MyComplex_1.h                                    # 原始头文件
├── bitwidth_optimization.py                         # 优化脚本
├── hls_tcl_generator.py                             # TCL生成器
│
├── bitwidth_config_8x8_16QAM_SNR20.json            # 输出: 配置文件 ✅
├── MyComplex_optimized_8x8_16QAM_SNR20.h           # 输出: 优化头文件 ✅
│
├── build_baseline.log                               # 日志: 基准测试 ✅
├── build_MyComplex_H_W38.log                        # 日志: 各变量测试 ✅
├── build_MyComplex_H_W36.log
├── ...
│
└── bitwidth_opt_*/                                  # HLS项目文件夹 (可清理)
    └── solution1/
        └── csim/
```

### 4. 执行时间估算

对于45个变量，每个变量测试约3-5次位宽配置：

- **单次Csim时间**: 2-5分钟
- **每个变量**: 10-25分钟
- **全部45个变量**: 7.5-18.75小时

**加速方法**:
1. 增大步长 (step=4而非step=2)
2. 只优化关键变量
3. 并行运行多个配置

### 5. 使用优化结果

#### 方法1: 直接替换
```bash
# 备份原始文件
cp MyComplex_1.h MyComplex_1.h.original

# 使用优化后的头文件
cp MyComplex_optimized_8x8_16QAM_SNR20.h MyComplex_1.h

# 运行HLS综合
vitis_hls -f your_synthesis_script.tcl
```

#### 方法2: 条件编译
在代码中使用宏:
```cpp
#ifdef USE_OPTIMIZED_BITWIDTH
#include "MyComplex_optimized_8x8_16QAM_SNR20.h"
#else
#include "MyComplex_1.h"
#endif
```

编译时:
```bash
vitis_hls -f your_script.tcl -DUSE_OPTIMIZED_BITWIDTH
```

### 6. 验证优化结果

#### A. 功能验证
```bash
# 使用优化后的头文件运行Csim
cp MyComplex_optimized_8x8_16QAM_SNR20.h MyComplex_1.h
vitis_hls -f csim_test.tcl

# 检查BER是否在可接受范围
grep "FINAL_BER" build.log
```

#### B. 资源验证
```bash
# 运行综合
vitis_hls -f synthesis_script.tcl

# 比较资源使用
# 查看 <project>/solution1/syn/report/<top>_csynth.rpt
```

**预期资源节省**:
- DSP: 减少5-15%
- BRAM: 减少10-20%
- LUT/FF: 减少8-12%
- 延迟: 可能略有增加(5-10%)

### 7. 常见问题排查

#### 问题1: BER急剧升高
**症状**: 某个变量的BER远超阈值

**原因**: 该变量对精度敏感

**解决**: 
```python
# 在parse_header_file中调整最小位宽
if var_name == "MyComplex_sensitive_var":
    var_info['min_W'] = 36  # 提高最小位宽限制
```

#### 问题2: 优化时间过长
**症状**: 单个变量优化超过30分钟

**解决**:
```bash
# 增大步长
python3 bitwidth_optimization.py ... --step 4

# 或修改代码
self.optimize_single_variable(var_name, mimo_config, step=4)
```

#### 问题3: 日志中找不到BER
**症状**: 日志文件中没有FINAL_BER输出

**检查**: main_hw.cpp是否包含BER输出
```cpp
printf("FINAL_BER: %.8f\n", final_ber);
```

### 8. 批量优化示例

如果要优化多个MIMO配置:

```bash
# 8x8 16QAM SNR20
python3 bitwidth_optimization.py \
    --nt 8 --nr 8 --modulation 16 --snr 20 \
    --data-path /home/ggg_wufuqi/hls/MHGD/MHGD

# 8x8 64QAM SNR25
python3 bitwidth_optimization.py \
    --nt 8 --nr 8 --modulation 64 --snr 25 \
    --data-path /home/ggg_wufuqi/hls/MHGD/MHGD

# 4x4 16QAM SNR15
python3 bitwidth_optimization.py \
    --nt 4 --nr 4 --modulation 16 --snr 15 \
    --data-path /home/ggg_wufuqi/hls/MHGD/MHGD
```

每次执行会生成对应的配置文件和优化头文件。

### 9. 结果分析

使用生成的JSON文件进行分析:

```python
import json
import matplotlib.pyplot as plt

# 读取配置
with open('bitwidth_config_8x8_16QAM_SNR20.json', 'r') as f:
    config = json.load(f)

# 统计位宽减少
reductions = []
for var_name, var_info in config['variables'].items():
    reduction = var_info['init_W'] - var_info['current_W']
    reductions.append((var_name, reduction))

# 排序并显示
reductions.sort(key=lambda x: x[1], reverse=True)
print("位宽减少最多的10个变量:")
for var_name, reduction in reductions[:10]:
    print(f"  {var_name}: {reduction} bits")
```

### 10. 总结

**执行脚本后，你将获得**:
1. ✅ JSON配置文件 - 记录完整优化过程
2. ✅ 优化后的头文件 - 可直接使用
3. ✅ 详细的日志文件 - 供调试分析
4. ✅ 终端输出摘要 - 快速了解结果

**下一步行动**:
1. 使用优化后的头文件进行HLS综合
2. 验证资源使用和时序
3. 运行RTL Cosim确认功能正确性
4. 根据需要调整BER阈值重新优化
