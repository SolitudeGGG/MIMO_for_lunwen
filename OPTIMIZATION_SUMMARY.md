# MHGD HLS 优化汇总

本文件集中列出本次对 `MHGD_accel_hw.cpp` 的性能优化点，便于统一查看。

## 1. 顶层 Dataflow 与接口优化
- **移除 `data_distribution` 串行分发**：在 `MHGD_detect_accel_hw` 内直接通过 `H_DISTRIBUTE / Y_DISTRIBUTE / V_DISTRIBUTE` 循环进行并行 fanout，内部 `for (s)` 使用 `#pragma HLS UNROLL`，避免串行瓶颈。
- **Stream 深度提升**：`H_real_stream/H_imag_stream/y_real_stream/y_imag_stream/v_tb_*_stream` 深度统一设为 256，提升输入 FIFO 缓冲能力，降低 dataflow 停顿风险。
- **流数组化**：将多组 Stream 以数组形式管理，减少接口重复并提升代码可维护性。

## 2. 核心计算模块 (sampler_task / samplers_process) 优化
- **任务级流水线**：在 `samplers_process` 的 `k` 循环外层使用 `#pragma HLS PIPELINE II=1`，实现每次 MCMC 迭代的阶段重叠执行。
- **函数调用开销消除**：对短小高频函数添加 `#pragma HLS INLINE`，包括：
  - `my_complex_add_hw`
  - `my_complex_add_hw_1`
  - `my_complex_scal_hw`
  - `my_complex_scal_hw_1`
  - `map_hw`

## 3. 矩阵乘法 (c_matmultiple_hw_pro) 专项优化
- **8×8 特化**：固定 `m/n/k = Ntr_1 (8)`，直接针对小矩阵实现。
- **全展开**：`i/j/l` 三层循环全部 `#pragma HLS UNROLL`，去除内部 pipeline。
- **完全分区**：对 `matA/matB/res` 使用 `#pragma HLS ARRAY_PARTITION complete`，确保访问并行化。
- **计算内融合**：不引入独立 Load/Store 阶段，直接在计算循环内读取分区数组并累加。

