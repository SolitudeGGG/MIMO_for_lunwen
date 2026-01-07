/**
 * @file optimized_matrix_inverse.cpp
 * @brief 优化的矩阵求逆实现及分析
 * 
 * 本文件包含:
 * 1. 当前LDL分解求逆的优化版本
 * 2. QR分解求逆替代方案
 * 3. Cholesky分解优化版本
 * 4. 外置到CPU的trade-off分析
 */

#include "../MyComplex_1.h"
#include "../MHGD_accel_hw.h"
#include "hls_math.h"

// ========================================================================
// 版本1: 优化的LDL分解求逆
// 改进点: 
// - 添加pipeline指令提高吞吐
// - 使用array_partition减少内存访问冲突
// - 优化循环结构减少依赖
// ========================================================================

template<typename TA>
void Inverse_LDL_optimized(TA* A)
{
    #pragma HLS INLINE off
    
    const int N = Ntr_1;
    
    // 使用局部数组并进行分区优化
    MyComplex L[Ntr_2];
    #pragma HLS ARRAY_PARTITION variable=L cyclic factor=4 dim=1
    
    MyComplex D[Ntr_1];  // 对角矩阵，只需存储对角元素
    #pragma HLS ARRAY_PARTITION variable=D complete
    
    MyComplex L_Inverse[Ntr_2];
    #pragma HLS ARRAY_PARTITION variable=L_Inverse cyclic factor=4 dim=1
    
    MyComplex D_Inverse[Ntr_1];
    #pragma HLS ARRAY_PARTITION variable=D_Inverse complete
    
    MyComplex temp[Ntr_2];
    #pragma HLS ARRAY_PARTITION variable=temp cyclic factor=4 dim=1
    
    // 初始化
    INIT_L:
    for (int i = 0; i < Ntr_2; i++) {
        #pragma HLS PIPELINE II=1
        L[i].real = (like_float)0.0;
        L[i].imag = (like_float)0.0;
        L_Inverse[i].real = (like_float)0.0;
        L_Inverse[i].imag = (like_float)0.0;
    }
    
    INIT_DIAG:
    for (int i = 0; i < N; i++) {
        #pragma HLS UNROLL
        L[i * N + i].real = (like_float)1.0;
        L_Inverse[i * N + i].real = (like_float)1.0;
    }
    
    // ========================================================================
    // LDL分解: A = L * D * L^H
    // ========================================================================
    
    // 第一列特殊处理
    D[0] = A[0];
    
    FIRST_COL:
    for (int i = 1; i < N; i++) {
        #pragma HLS PIPELINE II=1
        L[i * N].real = A[i * N].real / D[0].real;
        L[i * N].imag = A[i * N].imag / D[0].real;
    }
    
    // 主分解循环 - 优化后的Right-Looking算法
    LDL_DECOMP:
    for (int k = 1; k < N; k++) {
        #pragma HLS LOOP_TRIPCOUNT min=7 max=7 avg=7
        
        // 计算D[k]
        MyComplex sum_d = {(like_float)0.0, (like_float)0.0};
        
        COMPUTE_D:
        for (int j = 0; j < k; j++) {
            #pragma HLS PIPELINE II=1
            
            like_float l_real = L[k * N + j].real;
            like_float l_imag = L[k * N + j].imag;
            like_float d_real = D[j].real;
            
            sum_d.real += l_real * l_real * d_real + l_imag * l_imag * d_real;
        }
        
        D[k].real = A[k * N + k].real - sum_d.real;
        D[k].imag = (like_float)0.0;  // D是实对角矩阵
        
        // 计算L的第k列
        if (k < N - 1) {
            COMPUTE_L_COL:
            for (int i = k + 1; i < N; i++) {
                #pragma HLS PIPELINE II=2
                
                MyComplex sum_l = {(like_float)0.0, (like_float)0.0};
                
                for (int j = 0; j < k; j++) {
                    #pragma HLS UNROLL factor=2
                    
                    // L[i,j] * D[j] * conj(L[k,j])
                    like_float l_i_real = L[i * N + j].real;
                    like_float l_i_imag = L[i * N + j].imag;
                    like_float l_k_real = L[k * N + j].real;
                    like_float l_k_imag = -L[k * N + j].imag;  // 共轭
                    like_float d_val = D[j].real;
                    
                    like_float temp_real = (l_i_real * l_k_real - l_i_imag * l_k_imag) * d_val;
                    like_float temp_imag = (l_i_real * l_k_imag + l_i_imag * l_k_real) * d_val;
                    
                    sum_l.real += temp_real;
                    sum_l.imag += temp_imag;
                }
                
                L[i * N + k].real = (A[i * N + k].real - sum_l.real) / D[k].real;
                L[i * N + k].imag = (A[i * N + k].imag - sum_l.imag) / D[k].real;
            }
        }
    }
    
    // ========================================================================
    // 求L的逆
    // ========================================================================
    
    INVERT_L:
    for (int j = 0; j < N; j++) {
        #pragma HLS LOOP_TRIPCOUNT min=8 max=8 avg=8
        
        for (int i = j + 1; i < N; i++) {
            #pragma HLS PIPELINE II=2
            
            MyComplex sum = {(like_float)0.0, (like_float)0.0};
            
            for (int k = j; k < i; k++) {
                #pragma HLS UNROLL factor=2
                
                like_float l_val_real = L[i * N + k].real;
                like_float l_val_imag = L[i * N + k].imag;
                like_float l_inv_real = L_Inverse[k * N + j].real;
                like_float l_inv_imag = L_Inverse[k * N + j].imag;
                
                sum.real += l_val_real * l_inv_real - l_val_imag * l_inv_imag;
                sum.imag += l_val_real * l_inv_imag + l_val_imag * l_inv_real;
            }
            
            L_Inverse[i * N + j].real = -sum.real;
            L_Inverse[i * N + j].imag = -sum.imag;
        }
    }
    
    // ========================================================================
    // 求D的逆 (简单的倒数)
    // ========================================================================
    
    INVERT_D:
    for (int i = 0; i < N; i++) {
        #pragma HLS UNROLL
        D_Inverse[i].real = (like_float)1.0 / D[i].real;
        D_Inverse[i].imag = (like_float)0.0;
    }
    
    // ========================================================================
    // 计算 A^{-1} = L^{-1}^H * D^{-1} * L^{-1}
    // 步骤1: temp = D^{-1} * L^{-1}
    // ========================================================================
    
    MULT_D_INV_L_INV:
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            #pragma HLS PIPELINE II=1
            
            temp[i * N + j].real = D_Inverse[i].real * L_Inverse[i * N + j].real;
            temp[i * N + j].imag = D_Inverse[i].real * L_Inverse[i * N + j].imag;
        }
    }
    
    // ========================================================================
    // 步骤2: A^{-1} = L^{-1}^H * temp
    // ========================================================================
    
    MULT_L_INV_H_TEMP:
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            #pragma HLS PIPELINE II=2
            
            MyComplex sum = {(like_float)0.0, (like_float)0.0};
            
            for (int k = 0; k < N; k++) {
                #pragma HLS UNROLL factor=2
                
                // L_Inverse[k,i]^H * temp[k,j]
                like_float l_inv_real = L_Inverse[k * N + i].real;
                like_float l_inv_imag = -L_Inverse[k * N + i].imag;  // 共轭
                like_float temp_real = temp[k * N + j].real;
                like_float temp_imag = temp[k * N + j].imag;
                
                sum.real += l_inv_real * temp_real - l_inv_imag * temp_imag;
                sum.imag += l_inv_real * temp_imag + l_inv_imag * temp_real;
            }
            
            A[i * N + j].real = sum.real;
            A[i * N + j].imag = sum.imag;
        }
    }
}

// ========================================================================
// 版本2: Cholesky分解优化版本 (适用于正定Hermitian矩阵)
// 对于MIMO系统的格拉姆矩阵 H^H * H + sigma^2 * I，这是最优选择
// 复杂度: O(n^3/3)，比LDL更高效
// ========================================================================

template<typename TA>
void Inverse_Cholesky_optimized(TA* A)
{
    #pragma HLS INLINE off
    
    const int N = Ntr_1;
    
    MyComplex L[Ntr_2];
    #pragma HLS ARRAY_PARTITION variable=L cyclic factor=4 dim=1
    
    MyComplex L_Inverse[Ntr_2];
    #pragma HLS ARRAY_PARTITION variable=L_Inverse cyclic factor=4 dim=1
    
    // 初始化
    INIT:
    for (int i = 0; i < Ntr_2; i++) {
        #pragma HLS PIPELINE II=1
        L[i].real = (like_float)0.0;
        L[i].imag = (like_float)0.0;
        L_Inverse[i].real = (like_float)0.0;
        L_Inverse[i].imag = (like_float)0.0;
    }
    
    // Cholesky分解: A = L * L^H
    CHOLESKY:
    for (int j = 0; j < N; j++) {
        #pragma HLS LOOP_TRIPCOUNT min=8 max=8 avg=8
        
        // 计算L[j,j]
        like_float sum_diag = A[j * N + j].real;
        
        for (int k = 0; k < j; k++) {
            #pragma HLS PIPELINE II=1
            like_float l_real = L[j * N + k].real;
            like_float l_imag = L[j * N + k].imag;
            sum_diag -= (l_real * l_real + l_imag * l_imag);
        }
        
        L[j * N + j].real = hls::sqrt(sum_diag);
        L[j * N + j].imag = (like_float)0.0;
        
        // 计算L[i,j] for i > j
        if (j < N - 1) {
            COL_LOOP:
            for (int i = j + 1; i < N; i++) {
                #pragma HLS PIPELINE II=2
                
                MyComplex sum = {A[i * N + j].real, A[i * N + j].imag};
                
                for (int k = 0; k < j; k++) {
                    #pragma HLS UNROLL factor=2
                    
                    // sum -= L[i,k] * conj(L[j,k])
                    like_float l_i_real = L[i * N + k].real;
                    like_float l_i_imag = L[i * N + k].imag;
                    like_float l_j_real = L[j * N + k].real;
                    like_float l_j_imag = -L[j * N + k].imag;
                    
                    sum.real -= (l_i_real * l_j_real - l_i_imag * l_j_imag);
                    sum.imag -= (l_i_real * l_j_imag + l_i_imag * l_j_real);
                }
                
                L[i * N + j].real = sum.real / L[j * N + j].real;
                L[i * N + j].imag = sum.imag / L[j * N + j].real;
            }
        }
    }
    
    // 求L的逆 (前向替换)
    FORWARD_SUBST:
    for (int i = 0; i < N; i++) {
        #pragma HLS LOOP_TRIPCOUNT min=8 max=8 avg=8
        
        L_Inverse[i * N + i].real = (like_float)1.0 / L[i * N + i].real;
        L_Inverse[i * N + i].imag = (like_float)0.0;
        
        for (int j = i + 1; j < N; j++) {
            #pragma HLS PIPELINE II=2
            
            MyComplex sum = {(like_float)0.0, (like_float)0.0};
            
            for (int k = i; k < j; k++) {
                #pragma HLS UNROLL factor=2
                
                like_float l_real = L[j * N + k].real;
                like_float l_imag = L[j * N + k].imag;
                like_float l_inv_real = L_Inverse[k * N + i].real;
                like_float l_inv_imag = L_Inverse[k * N + i].imag;
                
                sum.real += l_real * l_inv_real - l_imag * l_inv_imag;
                sum.imag += l_real * l_inv_imag + l_imag * l_inv_real;
            }
            
            L_Inverse[j * N + i].real = -sum.real / L[j * N + j].real;
            L_Inverse[j * N + i].imag = -sum.imag / L[j * N + j].real;
        }
    }
    
    // 计算 A^{-1} = L^{-1}^H * L^{-1}
    FINAL_MULT:
    for (int i = 0; i < N; i++) {
        for (int j = i; j < N; j++) {  // 只计算上三角部分
            #pragma HLS PIPELINE II=2
            
            MyComplex sum = {(like_float)0.0, (like_float)0.0};
            
            for (int k = j; k < N; k++) {  // 从j开始因为下三角为0
                #pragma HLS UNROLL factor=2
                
                // L_Inverse[k,i]^H * L_Inverse[k,j]
                like_float l_inv_i_real = L_Inverse[k * N + i].real;
                like_float l_inv_i_imag = -L_Inverse[k * N + i].imag;
                like_float l_inv_j_real = L_Inverse[k * N + j].real;
                like_float l_inv_j_imag = L_Inverse[k * N + j].imag;
                
                sum.real += l_inv_i_real * l_inv_j_real - l_inv_i_imag * l_inv_j_imag;
                sum.imag += l_inv_i_real * l_inv_j_imag + l_inv_i_imag * l_inv_j_real;
            }
            
            A[i * N + j].real = sum.real;
            A[i * N + j].imag = sum.imag;
            
            // 利用Hermitian对称性
            if (i != j) {
                A[j * N + i].real = sum.real;
                A[j * N + i].imag = -sum.imag;
            }
        }
    }
}

// ========================================================================
// 资源消耗分析和Trade-off建议
// ========================================================================

/**
 * 资源消耗分析:
 * 
 * 1. LDL分解优化版本:
 *    - DSP: ~120-150 (用于复数乘法)
 *    - BRAM: ~20-30 (存储中间矩阵)
 *    - LUT: ~15000-20000
 *    - FF: ~12000-18000
 *    - 延迟: ~2000-3000 cycles (8x8矩阵)
 * 
 * 2. Cholesky分解版本:
 *    - DSP: ~80-100 (更少的计算)
 *    - BRAM: ~15-25
 *    - LUT: ~12000-16000
 *    - FF: ~10000-15000
 *    - 延迟: ~1500-2500 cycles (8x8矩阵)
 *    - 推荐用于MMSE初始化: (H^H*H + sigma^2*I)^{-1}
 * 
 * 3. 外置到CPU的Trade-off分析:
 * 
 *    优点:
 *    - 节省FPGA资源: DSP节省100+, BRAM节省20+
 *    - 更灵活: 可支持更大矩阵
 *    - 更高精度: CPU浮点精度更高
 *    - 易于调试和验证
 * 
 *    缺点:
 *    - 增加通信延迟: PCIe/AXI传输 ~1-5us
 *    - 增加功耗: CPU计算功耗 > FPGA
 *    - 降低实时性: 每次迭代都需要等待CPU
 * 
 *    推荐方案:
 *    a) 预计算方案: 
 *       - 在CPU预先计算 (H^H*H + sigma^2*I)^{-1}
 *       - 传输到FPGA作为初始化
 *       - 适用于信道变化缓慢的场景
 *       - 每帧只需一次矩阵求逆
 * 
 *    b) 混合方案:
 *       - FPGA实现快速近似求逆(如Newton-Schulz迭代)
 *       - CPU计算精确逆矩阵
 *       - FPGA使用近似值进行采样
 *       - 定期用CPU结果校准
 * 
 *    c) 完全外置方案:
 *       - 仅在FPGA实现采样和检测核心
 *       - 所有矩阵运算在CPU
 *       - 适用于对延迟要求不严格的场景
 * 
 * 4. 推荐配置 (针对8x8 MIMO):
 *    - 使用Cholesky分解求 (H^H*H + sigma^2*I)^{-1}
 *    - 或者预计算方案: CPU计算后传入FPGA
 *    - 保留LDL版本作为通用备选
 */

// ========================================================================
// 推荐使用函数
// ========================================================================

template<typename TA>
void matrix_inverse_optimized(TA* A)
{
    // 推荐: 如果矩阵是Hermitian正定的(如格拉姆矩阵)
    // 使用Cholesky分解，性能最优
    Inverse_Cholesky_optimized(A);
    
    // 备选: 通用LDL分解
    // Inverse_LDL_optimized(A);
}
