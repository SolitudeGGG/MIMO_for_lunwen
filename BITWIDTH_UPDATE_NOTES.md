# 位宽优化脚本更新说明

## 更新内容

### 1. 使用Vitis HLS进行Csim测试

原有的`bitwidth_optimization.py`使用g++编译测试，现已更新为使用Vitis HLS的Csim流程，与用户提供的TCL脚本保持一致。

### 2. 新增文件

#### `hls_tcl_generator.py`
HLS TCL脚本生成器，包含两个主要函数：

- **`generate_full_hls_tcl()`**: 生成Csim专用TCL脚本
- **`generate_synthesis_tcl()`**: 生成包含综合的完整TCL脚本

支持自动配置：
- 测试数据文件路径
- FPGA器件型号
- 时钟周期
- 接口配置

#### 更新的`bitwidth_optimization.py`

主要改进：
1. **HLS集成**: 使用`vitis_hls`命令行工具运行TCL脚本
2. **日志捕获**: 将HLS输出保存到`build_<config>.log`文件
3. **BER提取**: 从日志文件中提取`FINAL_BER`或`BER`关键字
4. **路径配置**: 新增`--data-path`参数指定测试数据路径

### 3. 使用方法

#### 基本用法

```bash
# 使用默认路径
python3 bitwidth_optimization.py --nt 8 --nr 8 --modulation 16 --snr 20

# 指定数据路径
python3 bitwidth_optimization.py \
    --nt 8 --nr 8 \
    --modulation 16 \
    --snr 20 \
    --data-path /home/ggg_wufuqi/hls/MHGD/MHGD
```

#### 高级选项

```bash
python3 bitwidth_optimization.py \
    --header MyComplex_1.h \
    --template sensitivity_types.hpp.jinja2 \
    --ber-threshold 1e-4 \
    --nt 8 --nr 8 \
    --modulation 16 \
    --snr 20 \
    --data-path /path/to/test/data \
    --output-config my_config.json \
    --output-header MyComplex_opt.h \
    --order high_to_low
```

参数说明：
- `--data-path`: HLS测试数据的基础路径（包含input_file, output_file等子目录）
- `--ber-threshold`: BER差异阈值（默认1e-4）
- `--order`: 优化顺序
  - `high_to_low`: 优先优化高位宽变量（推荐）
  - `low_to_high`: 优先优化低位宽变量
  - `sequential`: 按定义顺序

### 4. 工作流程

1. **生成TCL脚本**: 根据配置自动生成HLS TCL脚本
2. **替换头文件**: 临时替换`MyComplex_1.h`为测试配置
3. **运行Csim**: 执行`vitis_hls -f <tcl_script>`
4. **提取BER**: 从`build_*.log`中提取BER值
5. **恢复环境**: 恢复原始头文件，清理临时文件

### 5. 输出文件

#### 日志文件
- `build_<config_name>.log`: HLS Csim完整日志
- 保留供调试使用

#### 配置文件
- `bitwidth_config_*.json`: 优化后的位宽配置
- 包含基准BER、优化参数、所有变量的位宽

#### 头文件
- `MyComplex_optimized_*.h`: 优化后的类型定义头文件

### 6. HLS项目管理

运行过程中会创建临时HLS项目：
```
bitwidth_opt_<config_name>/
├── solution1/
│   ├── csim/
│   │   └── build/
│   └── ...
└── ...
```

这些项目文件夹可以手动清理：
```bash
rm -rf bitwidth_opt_*
```

### 7. 故障排除

#### 问题1: 找不到vitis_hls命令

**原因**: Vitis HLS未安装或未添加到PATH

**解决**:
```bash
# 添加Vitis HLS到PATH
export PATH=/tools/Xilinx/Vitis_HLS/2023.1/bin:$PATH

# 或使用完整路径
alias vitis_hls=/tools/Xilinx/Vitis_HLS/2023.1/bin/vitis_hls
```

#### 问题2: 无法提取BER

**原因**: main_hw.cpp中没有输出`FINAL_BER`标记

**解决**: 确保main_hw.cpp包含如下输出：
```cpp
printf("FINAL_BER: %.8f\n", BER);
```

#### 问题3: 测试数据文件未找到

**原因**: `--data-path`参数不正确

**检查**: 确保路径包含以下目录结构：
```
<data-path>/
├── input_file/
│   ├── H_SNR=*.txt
│   └── y_SNR=*.txt
├── output_file/
│   └── bits_output_SNR=*.txt
├── _16QAM_Constellation.txt
├── _64QAM_Constellation.txt
└── gaussian_random_values*.txt
```

#### 问题4: HLS执行超时

**原因**: Csim时间过长（>10分钟）

**解决**: 
- 减少测试数据量
- 调整超时时间（修改`timeout=600`参数）

### 8. 性能优化建议

#### 加速优化过程

1. **并行运行**: 使用不同配置并行优化
```bash
# Terminal 1
python3 bitwidth_optimization.py --snr 5 &

# Terminal 2
python3 bitwidth_optimization.py --snr 10 &
```

2. **减少变量数**: 只优化关键变量
```python
# 修改parse_header_file()，过滤不重要的变量
```

3. **增大步长**: 使用`step=4`代替`step=2`
```python
# 在optimize_single_variable中
self.optimize_single_variable(var_name, mimo_config, step=4)
```

### 9. 集成到批量优化

更新`batch_optimization.py`时确保传递`--data-path`参数：

```python
cmd = [
    "python3", "bitwidth_optimization.py",
    "--nt", str(nt),
    "--nr", str(nr),
    "--modulation", str(modulation),
    "--snr", str(snr),
    "--data-path", data_base_path,  # 新增
    "--output-config", output_config,
    "--output-header", output_header,
    "--order", "high_to_low"
]
```

### 10. 示例：完整工作流

```bash
# 步骤1: 准备测试数据
python3 generate_mimo_data.py

# 步骤2: 运行单配置优化测试
python3 bitwidth_optimization.py \
    --nt 8 --nr 8 \
    --modulation 16 \
    --snr 20 \
    --data-path ./mimo_data \
    --ber-threshold 1e-4

# 步骤3: 检查结果
cat build_baseline.log | grep "FINAL_BER"
cat bitwidth_config_8x8_16QAM_SNR20.json

# 步骤4: 使用优化后的头文件
cp MyComplex_optimized_8x8_16QAM_SNR20.h MyComplex_1.h
vitis_hls -f your_synthesis_script.tcl
```

## 技术细节

### TCL脚本模板

生成的TCL脚本结构：
```tcl
open_project -reset <project_name>
add_files <design_files>
add_files -tb <testbench_files>
add_files -tb <data_files> -cflags "-I."
set_top <top_function>
open_solution -reset "solution1"
set_part {<fpga_part>}
create_clock -period <period>
config_interface ...
csim_design -ldflags "-lm"
exit
```

### BER提取逻辑

使用正则表达式从日志中提取：
```python
# 方法1: 精确匹配
ber_match = re.search(r'FINAL_BER:\s*([\d.e+-]+)', log_content)

# 方法2: 宽松匹配
ber_match = re.search(r'BER\s*[=:]\s*([\d.e+-]+)', log_content, re.IGNORECASE)
```

### 环境恢复机制

使用try-finally确保环境清理：
```python
try:
    # 测试代码
finally:
    # 恢复原始头文件
    # 清理临时文件
```

## 常见问题

**Q: 是否支持其他HLS工具？**
A: 当前只支持Xilinx Vitis HLS。其他工具需要修改TCL生成逻辑。

**Q: 可以跳过某些变量的优化吗？**
A: 可以在`parse_header_file()`中添加过滤逻辑。

**Q: 如何加快优化速度？**
A: 增大步长、减少测试样本、并行运行。

**Q: 优化结果是否需要手动验证？**
A: 建议在实际HLS综合后验证资源使用和时序。
