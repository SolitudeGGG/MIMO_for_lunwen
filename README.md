# MIMO_for_lunwen

## 固定点位宽自动优化系统

本项目已完整实现MIMO HLS固定点位宽自动优化系统。

### 快速开始

```bash
# 安装依赖
pip install -r requirements.txt

# 运行完整优化流程
python3 master_optimization.py

# 或运行系统测试
python3 test_system.py
```

### 核心功能

1. ✅ 使用commpy生成MIMO测试数据（4/16/64QAM，SNR 5-25dB）
2. ✅ 自动化位宽优化（45个变量，基于BER性能）
3. ✅ 批量生成15个配置的优化头文件
4. ✅ 子函数变量分析（77个函数）
5. ✅ 完整的自动化流程和测试

### 详细文档

- **[使用指南](README_OPTIMIZATION.md)** - 完整的使用文档和API说明
- **[系统架构](ARCHITECTURE.md)** - 系统架构图和流程说明
- **[项目总结](SUMMARY.md)** - 技术细节和实现说明
- **[交付清单](DELIVERABLES.md)** - 完整的文件和功能清单
- **[问题清单](questions.md)** - 需要确认的事项和常见问题

### 文件说明

| 脚本 | 功能 |
|------|------|
| `generate_mimo_data.py` | MIMO数据生成器 |
| `bitwidth_optimization.py` | 位宽优化引擎 |
| `batch_optimization.py` | 批量优化处理 |
| `subfunction_analyzer.py` | 子函数分析工具 |
| `master_optimization.py` | 主控脚本 |
| `test_system.py` | 系统测试工具 |

### 原始HLS项目

- `MHGD_accel_hw.cpp/h` - HLS加速器实现
- `MyComplex_1.h` - 固定点类型定义
- `main_hw.cpp` - 测试主程序
- `sensitivity_types.hpp.jinja2` - Jinja2模板
- `纯动态驱动的定点位宽优化流程.pdf` - 优化流程文档