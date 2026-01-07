/**
 * @file optimized_matrix_multiply.cpp
 * @brief 优化的复数矩阵乘法实现
 * 
 * 本文件包含多种HLS优化的矩阵乘法实现：
 * 1. Pipeline优化版本 - 基础流水线优化
 * 2. Dataflow优化版本 - 使用数据流架构
 * 3. Array Partition优化版本 - 内存分区优化
 * 4. Systolic Array版本 - 脉动阵列架构（适用于大规模矩阵）
 */

#include "../MyComplex_1.h"
#include "../MHGD_accel_hw.h"
#include "hls_math.h"

// ========================================================================
// 版本1: Pipeline优化版本
// 特点: 在最内层循环应用pipeline，减少II
// 适用场景: 中小规模矩阵 (8x8)
// 预期性能: 延迟降低约40-50%, LUT增加10-15%
// ========================================================================
template<typename TA, typename TB, typename TR>
void matmul_pipeline_opt(
    TA* matA, int transA,
    TB* matB, int transB,
    int ma, int na, int mb, int nb,
    TR* res)
{
    #pragma HLS INLINE off
    
    int m = (transA == 0) ? ma : na;
    int k = (transA == 0) ? na : ma;
    int n = (transB == 0) ? nb : mb;
    
    // 外层循环 - 结果矩阵的行
    ROW_LOOP:
    for (int i = 0; i < m; i++) {
        #pragma HLS LOOP_TRIPCOUNT min=8 max=8 avg=8
        
        // 中层循环 - 结果矩阵的列
        COL_LOOP:
        for (int j = 0; j < n; j++) {
            #pragma HLS LOOP_TRIPCOUNT min=8 max=8 avg=8
            // 关键优化: Pipeline整个j循环，允许并行处理多列
            #pragma HLS PIPELINE II=1
            
            MyComplex sum = {(like_float)0.0, (like_float)0.0};
            
            // 内层循环 - 点积计算
            DOT_PRODUCT_LOOP:
            for (int l = 0; l < k; l++) {
                #pragma HLS LOOP_TRIPCOUNT min=8 max=8 avg=8
                // 完全展开以实现并行累加
                #pragma HLS UNROLL factor=2
                
                TA a_element;
                TB b_element;
                
                // 获取A元素
                if (transA == 1) {
                    a_element.real = matA[l * na + i].real;
                    a_element.imag = -matA[l * na + i].imag;
                } else {
                    a_element = matA[i * na + l];
                }
                
                // 获取B元素
                if (transB == 0) {
                    b_element = matB[l * nb + j];
                } else {
                    b_element.real = matB[j * nb + l].real;
                    b_element.imag = -matB[j * nb + l].imag;
                }
                
                // 复数乘法和累加
                like_float temp_real = a_element.real * b_element.real - 
                                       a_element.imag * b_element.imag;
                like_float temp_imag = a_element.real * b_element.imag + 
                                       a_element.imag * b_element.real;
                
                sum.real += temp_real;
                sum.imag += temp_imag;
            }
            
            res[i * n + j].real = sum.real;
            res[i * n + j].imag = sum.imag;
        }
    }
}

// ========================================================================
// 版本2: Array Partition优化版本
// 特点: 使用数组分区提高内存带宽
// 适用场景: 中小规模矩阵，内存访问密集
// 预期性能: 吞吐量提升2-3倍，BRAM使用增加
// ========================================================================
template<typename TA, typename TB, typename TR>
void matmul_partition_opt(
    TA* matA, int transA,
    TB* matB, int transB,
    int ma, int na, int mb, int nb,
    TR* res)
{
    #pragma HLS INLINE off
    
    const int MAX_SIZE = 8;
    
    // 局部缓存，完全分区以支持并行访问
    TA A_local[MAX_SIZE][MAX_SIZE];
    #pragma HLS ARRAY_PARTITION variable=A_local complete dim=2
    
    TB B_local[MAX_SIZE][MAX_SIZE];
    #pragma HLS ARRAY_PARTITION variable=B_local complete dim=1
    
    TR C_local[MAX_SIZE][MAX_SIZE];
    #pragma HLS ARRAY_PARTITION variable=C_local complete dim=0
    
    int m = (transA == 0) ? ma : na;
    int k = (transA == 0) ? na : ma;
    int n = (transB == 0) ? nb : mb;
    
    // 数据加载阶段 - 加载A矩阵到本地缓存
    LOAD_A:
    for (int i = 0; i < m; i++) {
        for (int l = 0; l < k; l++) {
            #pragma HLS PIPELINE II=1
            if (transA == 0) {
                A_local[i][l] = matA[i * na + l];
            } else {
                A_local[i][l].real = matA[l * na + i].real;
                A_local[i][l].imag = -matA[l * na + i].imag;
            }
        }
    }
    
    // 数据加载阶段 - 加载B矩阵到本地缓存
    LOAD_B:
    for (int l = 0; l < k; l++) {
        for (int j = 0; j < n; j++) {
            #pragma HLS PIPELINE II=1
            if (transB == 0) {
                B_local[l][j] = matB[l * nb + j];
            } else {
                B_local[l][j].real = matB[j * nb + l].real;
                B_local[l][j].imag = -matB[j * nb + l].imag;
            }
        }
    }
    
    // 计算阶段 - 使用分区后的数组进行并行计算
    COMPUTE:
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            #pragma HLS PIPELINE II=1
            
            MyComplex sum = {(like_float)0.0, (like_float)0.0};
            
            for (int l = 0; l < k; l++) {
                #pragma HLS UNROLL factor=4
                
                like_float temp_real = A_local[i][l].real * B_local[l][j].real - 
                                       A_local[i][l].imag * B_local[l][j].imag;
                like_float temp_imag = A_local[i][l].real * B_local[l][j].imag + 
                                       A_local[i][l].imag * B_local[l][j].real;
                
                sum.real += temp_real;
                sum.imag += temp_imag;
            }
            
            C_local[i][j].real = sum.real;
            C_local[i][j].imag = sum.imag;
        }
    }
    
    // 数据存储阶段 - 将结果写回
    STORE_C:
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            #pragma HLS PIPELINE II=1
            res[i * n + j] = C_local[i][j];
        }
    }
}

// ========================================================================
// 版本3: Dataflow优化版本
// 特点: 使用数据流架构，将加载-计算-存储流水化
// 适用场景: 需要高吞吐量的场景
// 预期性能: 吞吐量提升3-4倍，延迟增加
// ========================================================================
template<typename TA, typename TB, typename TR>
void matmul_dataflow_load(
    TA* matA, int transA, int m, int k, int na,
    hls::stream<TA> &A_stream)
{
    for (int i = 0; i < m; i++) {
        for (int l = 0; l < k; l++) {
            #pragma HLS PIPELINE II=1
            TA elem;
            if (transA == 0) {
                elem = matA[i * na + l];
            } else {
                elem.real = matA[l * na + i].real;
                elem.imag = -matA[l * na + i].imag;
            }
            A_stream.write(elem);
        }
    }
}

template<typename TB>
void matmul_dataflow_load_B(
    TB* matB, int transB, int k, int n, int nb,
    hls::stream<TB> &B_stream)
{
    for (int l = 0; l < k; l++) {
        for (int j = 0; j < n; j++) {
            #pragma HLS PIPELINE II=1
            TB elem;
            if (transB == 0) {
                elem = matB[l * nb + j];
            } else {
                elem.real = matB[j * nb + l].real;
                elem.imag = -matB[j * nb + l].imag;
            }
            B_stream.write(elem);
        }
    }
}

template<typename TA, typename TB, typename TR>
void matmul_dataflow_compute(
    hls::stream<TA> &A_stream,
    hls::stream<TB> &B_stream,
    hls::stream<TR> &C_stream,
    int m, int n, int k)
{
    const int MAX_SIZE = 8;
    TB B_buffer[MAX_SIZE][MAX_SIZE];
    #pragma HLS ARRAY_PARTITION variable=B_buffer complete dim=2
    
    // 先加载完整的B矩阵
    for (int l = 0; l < k; l++) {
        for (int j = 0; j < n; j++) {
            #pragma HLS PIPELINE II=1
            B_buffer[l][j] = B_stream.read();
        }
    }
    
    // 计算C = A * B
    for (int i = 0; i < m; i++) {
        TA A_row[MAX_SIZE];
        #pragma HLS ARRAY_PARTITION variable=A_row complete
        
        // 读取A的一行
        for (int l = 0; l < k; l++) {
            #pragma HLS PIPELINE II=1
            A_row[l] = A_stream.read();
        }
        
        // 计算结果的一行
        for (int j = 0; j < n; j++) {
            #pragma HLS PIPELINE II=1
            
            MyComplex sum = {(like_float)0.0, (like_float)0.0};
            
            for (int l = 0; l < k; l++) {
                #pragma HLS UNROLL factor=4
                
                like_float temp_real = A_row[l].real * B_buffer[l][j].real - 
                                       A_row[l].imag * B_buffer[l][j].imag;
                like_float temp_imag = A_row[l].real * B_buffer[l][j].imag + 
                                       A_row[l].imag * B_buffer[l][j].real;
                
                sum.real += temp_real;
                sum.imag += temp_imag;
            }
            
            TR result;
            result.real = sum.real;
            result.imag = sum.imag;
            C_stream.write(result);
        }
    }
}

template<typename TR>
void matmul_dataflow_store(
    hls::stream<TR> &C_stream,
    TR* res, int m, int n)
{
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            #pragma HLS PIPELINE II=1
            res[i * n + j] = C_stream.read();
        }
    }
}

template<typename TA, typename TB, typename TR>
void matmul_dataflow_opt(
    TA* matA, int transA,
    TB* matB, int transB,
    int ma, int na, int mb, int nb,
    TR* res)
{
    #pragma HLS INLINE off
    #pragma HLS DATAFLOW
    
    int m = (transA == 0) ? ma : na;
    int k = (transA == 0) ? na : ma;
    int n = (transB == 0) ? nb : mb;
    
    hls::stream<TA> A_stream("A_stream");
    #pragma HLS STREAM variable=A_stream depth=64
    
    hls::stream<TB> B_stream("B_stream");
    #pragma HLS STREAM variable=B_stream depth=64
    
    hls::stream<TR> C_stream("C_stream");
    #pragma HLS STREAM variable=C_stream depth=64
    
    matmul_dataflow_load(matA, transA, m, k, na, A_stream);
    matmul_dataflow_load_B(matB, transB, k, n, nb, B_stream);
    matmul_dataflow_compute(A_stream, B_stream, C_stream, m, n, k);
    matmul_dataflow_store(C_stream, res, m, n);
}

// ========================================================================
// 版本4: 简化的脉动阵列版本
// 特点: 适用于固定大小矩阵的高并行度实现
// 适用场景: 8x8矩阵，追求最大吞吐量
// 预期性能: 延迟最低，资源消耗最高
// ========================================================================
template<typename TA, typename TB, typename TR>
void matmul_systolic_opt(
    TA* matA, int transA,
    TB* matB, int transB,
    int ma, int na, int mb, int nb,
    TR* res)
{
    #pragma HLS INLINE off
    
    const int N = 8;  // 固定为8x8矩阵
    
    // 使用完全分区的处理单元阵列
    MyComplex PE[N][N];
    #pragma HLS ARRAY_PARTITION variable=PE complete dim=0
    
    TA A_local[N][N];
    #pragma HLS ARRAY_PARTITION variable=A_local complete dim=0
    
    TB B_local[N][N];
    #pragma HLS ARRAY_PARTITION variable=B_local complete dim=0
    
    // 初始化
    INIT_PE:
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            #pragma HLS UNROLL
            PE[i][j].real = 0.0;
            PE[i][j].imag = 0.0;
        }
    }
    
    // 加载数据
    LOAD_DATA:
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            #pragma HLS UNROLL
            
            if (transA == 0) {
                A_local[i][j] = matA[i * na + j];
            } else {
                A_local[i][j].real = matA[j * na + i].real;
                A_local[i][j].imag = -matA[j * na + i].imag;
            }
            
            if (transB == 0) {
                B_local[i][j] = matB[i * nb + j];
            } else {
                B_local[i][j].real = matB[j * nb + i].real;
                B_local[i][j].imag = -matB[j * nb + i].imag;
            }
        }
    }
    
    // 脉动计算 - 完全展开
    SYSTOLIC_COMPUTE:
    for (int k = 0; k < N; k++) {
        #pragma HLS UNROLL
        
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                #pragma HLS UNROLL
                
                like_float temp_real = A_local[i][k].real * B_local[k][j].real - 
                                       A_local[i][k].imag * B_local[k][j].imag;
                like_float temp_imag = A_local[i][k].real * B_local[k][j].imag + 
                                       A_local[i][k].imag * B_local[k][j].real;
                
                PE[i][j].real += temp_real;
                PE[i][j].imag += temp_imag;
            }
        }
    }
    
    // 存储结果
    STORE_RESULT:
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            #pragma HLS UNROLL
            res[i * N + j].real = PE[i][j].real;
            res[i * N + j].imag = PE[i][j].imag;
        }
    }
}

// ========================================================================
// 推荐使用函数 - 根据场景选择最佳实现
// ========================================================================

/**
 * 推荐用于替换c_matmultiple_hw_pro的函数
 * 对于8x8 MIMO系统，推荐使用pipeline版本作为平衡选择
 */
template<typename TA, typename TB, typename TR>
void c_matmultiple_hw_optimized(
    TA* matA, int transA,
    TB* matB, int transB,
    int ma, int na, int mb, int nb,
    TR* res)
{
    // 默认使用pipeline优化版本
    // 可根据综合结果切换到其他版本
    matmul_pipeline_opt(matA, transA, matB, transB, ma, na, mb, nb, res);
    
    // 其他选项:
    // matmul_partition_opt(matA, transA, matB, transB, ma, na, mb, nb, res);
    // matmul_dataflow_opt(matA, transA, matB, transB, ma, na, mb, nb, res);
    // matmul_systolic_opt(matA, transA, matB, transB, ma, na, mb, nb, res);
}
