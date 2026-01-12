# MIMO Multi-Configuration Build System

本项目实现了支持多种MIMO配置的动态可重构电路设计系统。通过使用不同的ap_fixed精度定义，为不同规模的MIMO系统生成优化的xclbin文件。

## 目录结构

```
MIMO_for_lunwen/
├── headers/                          # 配置相关的头文件目录
│   ├── mimo_4x4_16QAM/              # 4x4 MIMO + 16QAM 配置
│   │   └── MyComplex_types.h
│   ├── mimo_8x8_4QAM/               # 8x8 MIMO + 4QAM 配置
│   │   └── MyComplex_types.h
│   ├── mimo_8x8_16QAM/              # 8x8 MIMO + 16QAM 配置
│   │   └── MyComplex_types.h
│   ├── mimo_8x8_64QAM/              # 8x8 MIMO + 64QAM 配置
│   │   └── MyComplex_types.h
│   ├── mimo_16x16_16QAM/            # 16x16 MIMO + 16QAM 配置
│   │   └── MyComplex_types.h
│   └── mimo_16x16_64QAM/            # 16x16 MIMO + 64QAM 配置
│       └── MyComplex_types.h
├── xclbin_host/                      # 构建和Host代码目录
│   ├── Makefile_multiconfig         # 支持多配置的Makefile
│   ├── build_mimo.sh                # 自动化构建脚本
│   ├── host.cpp                     # Host端代码
│   └── MHGD_compile.cfg             # Vitis编译配置
├── MHGD_accel_hw.cpp                # HLS内核源代码
└── MHGD_accel_hw.h                  # HLS内核头文件
```

## 支持的配置

系统支持以下6种MIMO配置，每种配置都针对特定的系统规模和调制方式优化了ap_fixed精度：

| 配置名称          | MIMO规模 | 调制方式 | SNR  | 说明                           |
|-------------------|----------|----------|------|--------------------------------|
| 4x4_16QAM         | 4x4      | 16QAM    | 25dB | 小规模MIMO，中等调制复杂度     |
| 8x8_4QAM          | 8x8      | 4QAM     | 25dB | 中等规模MIMO，低调制复杂度     |
| 8x8_16QAM         | 8x8      | 16QAM    | 25dB | 中等规模MIMO，中等调制复杂度   |
| 8x8_64QAM         | 8x8      | 64QAM    | 25dB | 中等规模MIMO，高调制复杂度     |
| 16x16_16QAM       | 16x16    | 16QAM    | 25dB | 大规模MIMO，中等调制复杂度     |
| 16x16_64QAM       | 16x16    | 64QAM    | 25dB | 大规模MIMO，高调制复杂度       |

## 构建流程

### 方法1: 使用自动化脚本（推荐）

```bash
cd xclbin_host

# 查看所有可用配置
./build_mimo.sh list

# 构建单个配置
./build_mimo.sh 8x8_16QAM

# 构建所有配置
./build_mimo.sh all
```

### 方法2: 使用Makefile

```bash
cd xclbin_host

# 查看可用配置
make -f Makefile_multiconfig list-configs

# 构建单个配置
make -f Makefile_multiconfig build MIMO_CONFIG=8x8_16QAM

# 构建所有配置
make -f Makefile_multiconfig build-all

# 清理构建产物
make -f Makefile_multiconfig clean
```

## 构建产物

构建完成后，文件将生成在 `xclbin_host/build/` 目录下：

```
xclbin_host/build/
├── 4x4_16QAM/
│   ├── kernel_4x4_16QAM.xclbin      # 可执行的bitstream文件
│   ├── temp/                         # 中间文件
│   └── reports/                      # 综合和实现报告
├── 8x8_4QAM/
│   └── kernel_8x8_4QAM.xclbin
├── 8x8_16QAM/
│   └── kernel_8x8_16QAM.xclbin
└── ... (其他配置)
```

## Host端使用

修改 `host.cpp` 以支持动态选择xclbin文件：

```cpp
#include <string>

// 根据MIMO配置选择对应的xclbin文件
std::string get_xclbin_path(int Nt, int Nr, int modulation_order) {
    std::string base_path = "./build/";
    std::string config;
    
    if (Nt == 4 && Nr == 4 && modulation_order == 16) {
        config = "4x4_16QAM";
    } else if (Nt == 8 && Nr == 8 && modulation_order == 4) {
        config = "8x8_4QAM";
    } else if (Nt == 8 && Nr == 8 && modulation_order == 16) {
        config = "8x8_16QAM";
    } else if (Nt == 8 && Nr == 8 && modulation_order == 64) {
        config = "8x8_64QAM";
    } else if (Nt == 16 && Nr == 16 && modulation_order == 16) {
        config = "16x16_16QAM";
    } else if (Nt == 16 && Nr == 16 && modulation_order == 64) {
        config = "16x16_64QAM";
    } else {
        throw std::runtime_error("Unsupported MIMO configuration");
    }
    
    return base_path + config + "/kernel_" + config + ".xclbin";
}

int main() {
    // 初始化FPGA设备
    int device_index = 0;
    auto device = xrt::device(device_index);
    
    // 根据系统参数动态选择xclbin
    int Nt = 8, Nr = 8, mod_order = 16;
    std::string xclbin_path = get_xclbin_path(Nt, Nr, mod_order);
    
    // 加载xclbin
    auto uuid = device.load_xclbin(xclbin_path);
    auto krnl = xrt::kernel(device, uuid, "MHGD_detect_accel_hw");
    
    // ... 后续Host代码 ...
}
```

## 精度配置说明

每个配置的头文件 (`MyComplex_types.h`) 包含了针对特定MIMO规模和调制方式优化的ap_fixed类型定义。主要优化变量包括：

- **H_real/H_imag**: 信道矩阵H的实部和虚部精度
- **y_real/y_imag**: 接收向量y的精度
- **x_prop_real/x_prop_imag**: 候选解向量的精度
- **r_norm**: 残差范数的精度
- **lr**: 学习率的精度
- 其他中间变量...

精度配置原则：
- 更大的MIMO规模需要更高的精度以避免累积误差
- 更高阶的调制方式（如64QAM）需要更精确的数值表示
- 在保证精度的前提下，尽量减少位宽以节省FPGA资源

## 关键技术难点

1. **资源约束平衡**: 不同配置的LUT/DSP/BRAM消耗差异大，需要根据FPGA平台容量调优
2. **接口一致性**: 确保所有配置的AXI接口定义一致，保证Host端兼容性
3. **时序收敛**: 高精度配置可能导致时序违例，需调整pipeline和时钟频率
4. **验证成本**: 每个配置都需要完整的功能和性能验证

## 进阶：部分重配置(Partial Reconfiguration)

当前实现是**静态多xclbin方案**（编译时重配置）。如需实现真正的动态部分重配置，需要：

1. 使用Vivado的DFX (Dynamic Function eXchange) 流程
2. 定义静态区域和可重配置区域
3. 创建多个可重配置模块(RM)
4. 使用ICAP接口实现运行时重配置

这比当前方案复杂得多，建议先验证静态方案可行后再考虑。

## 故障排查

### 构建失败
- 确认Xilinx Vitis工具已正确安装并设置环境变量
- 检查FPGA平台是否正确：`platforminfo -p xilinx_u50_gen3x16_xdma_5_202210_1`
- 查看构建日志：`xclbin_host/build/<config>/logs/`

### 资源不足
- 减少数据位宽
- 降低并行度
- 选择更小规模的MIMO配置

### 时序违例
- 降低时钟频率（修改Makefile中的CLOCK_FREQ_MHZ）
- 增加pipeline深度
- 优化数据路径

## 联系与支持

如有问题，请提交Issue或联系项目维护者。
