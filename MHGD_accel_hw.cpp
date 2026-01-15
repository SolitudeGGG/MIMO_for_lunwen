#include "MyComplex_1.h"
#include "hls_math.h"
#include "MHGD_accel_hw.h"
#include <stdio.h>
#include <string.h>
#include "hls_stream.h"


MyComplex QPSK_Constellation_hw[4] = {{-1,-1},{-1,1},{1,-1},{1,1}};
MyComplex _16QAM_Constellation_hw[16] = {{-3.0,-3.0},{-3.0,-1.0},{-3.0,3.0},{-3.0,1.0},
										 {-1.0,-3.0},{-1.0,-1.0},{-1.0,3.0},{-1.0,1.0},
										 {3.0,-3.0},{3.0,-1.0},{3.0,3.0},{3.0,1.0},
										 {1.0,-3.0},{1.0,-1.0},{1.0,3.0},{1.0,1.0}};
MyComplex _64QAM_Constellation_hw[64] = {{-7.0, 7.0},  {-5.0, 7.0},  {-3.0, 7.0}, {-1.0, 7.0}, //
                                         {1.0, 7.0},   {3.0, 7.0},   {5.0, 7.0},  {7.0, 7.0},  //
                                         {-7.0, 5.0},  {-5.0, 5.0},  {-3.0, 5.0}, {-1.0, 5.0}, //
                                         {1.0, 5.0},   {3.0, 5.0},   {5.0, 5.0},  {7.0, 5.0},  //
                                         {-7.0, 3.0},  {-5.0, 3.0},  {-3.0, 3.0}, {-1.0, 3.0}, //
                                         {1.0, 3.0},   {3.0, 3.0},   {5.0, 3.0},  {7.0, 3.0}, //
                                         {-7.0, 1.0},  {-5.0, 1.0},  {-3.0, 1.0}, {-1.0, 1.0}, //
                                         {1.0, 1.0},   {3.0, 1.0},   {5.0, 1.0},  {7.0, 1.0}, //
                                         {-7.0, -1.0}, {-5.0, -1.0}, {-3.0, -1.0},{-1.0, -1.0}, //
                                         {1.0, -1.0},  {3.0, -1.0},  {5.0, -1.0}, {7.0, -1.0}, //
                                         {-7.0, -3.0}, {-5.0, -3.0}, {-3.0, -3.0},{-1.0, -3.0}, //
                                         {1.0, -3.0},  {3.0, -3.0},  {5.0, -3.0}, {7.0, -3.0}, //
                                         {-7.0, -5.0}, {-5.0, -5.0}, {-3.0, -5.0},{-1.0, -5.0},//
                                         {1.0, -5.0},  {3.0, -5.0},  {5.0, -5.0}, {7.0, -5.0}, //
                                         {-7.0, -7.0}, {-5.0, -7.0}, {-3.0, -7.0},{-1.0, -7.0},
                                         {1.0, -7.0},  {3.0, -7.0},  {5.0, -7.0}, {7.0, -7.0}};


/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/
/***************************************部分供tb调用的C代码***********************************/
/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/
// 读取已经生成的高斯随机变量数据
void read_gaussian_data_hw(const char* filename, MyComplex_v* array, int n, int offset) {
	FILE* f = fopen(filename, "r");
	if (f == NULL) {
		printf("Error opening file!\n");
		return;  // 错误打开文件
	}

	int i;
	float temp_real, temp_imag;
	like_float local_temp = sqrt(2.0f);

	// 跳过文件中的前offset个数据，继续读取
	for (i = 0; i < offset; i++) {
		// 读取每一行的格式 "Real: <value>, Imaginary: <value>"
		if (fscanf(f, "Real: %f, Imaginary: %f\n", &temp_real, &temp_imag) != 2) {
			printf("Error reading data at offset %d!\n", i);
			fclose(f);
			return;  // 错误读取数据
		}
	}

	// 读取n个数据点
	for (i = 0; i < n; i++) {
		if (fscanf(f, "Real: %f, Imaginary: %f\n", &temp_real, &temp_imag) != 2) {
			printf("Error reading data!!\n");
			fclose(f);
			return;  // 返回已成功读取的数据数量
		}
		// 将读取的实部和虚部数据存入数组
		array[i].real = hls_internal::generic_divide((ap_fixed<40,8>)temp_real, (ap_fixed<40,8>)local_temp);
		array[i].imag = hls_internal::generic_divide((ap_fixed<40,8>)temp_imag, (ap_fixed<40,8>)local_temp);
	}
	fclose(f);
}

void QAM_Demodulation_hw(MyComplex* x_hat, int Nt, int mu, int* bits_demod)
{
	switch (mu)
	{
	case 2:
		QPSK_Demodulation_hw(x_hat, Nt, bits_demod);
		break;
	case 4:
		_16QAM_Demodulation_hw(x_hat, Nt, bits_demod);
		break;
	case 6:
		_64QAM_Demodulation_hw(x_hat, Nt, bits_demod);
		break;
	default:
		break;
	}
}

/*QPSK解调*/
void QPSK_Demodulation_hw(MyComplex* x_hat, int Nt, int* bits_demod)
{
	int i = 0; int j = 0; int best_id = 0;
	MyComplex temp[4];
	MyComplex temp_1[4];
	like_float temp_2;
	like_float distance[4] = {0, 0, 0, 0};
	/*for one in x_hat*/
	for (i = 0; i < Nt; i++)
	{
		for (j = 0; j < 4; j++)
		{
			temp_1[j].real = x_hat[i].real * hls::sqrt((ap_fixed<40,8>)2.0);
			temp_1[j].imag = x_hat[i].imag * hls::sqrt((ap_fixed<40,8>)2.0);
			temp[j].real = temp_1[j].real - QPSK_Constellation_hw[j].real;
			temp[j].imag = temp_1[j].imag - QPSK_Constellation_hw[j].imag;
		}
		for (j = 0; j < 4; j++)
		{
			temp_2 = (temp[j].real * temp[j].real + temp[j].imag * temp[j].imag);
			distance[j] = hls::sqrt((ap_fixed<40,8>)temp_2);
		}
		best_id = argmin_hw(distance, 4);
		switch (best_id)
		{
		case 0:
			bits_demod[2 * i] = 0; bits_demod[2 * i + 1] = 0;
			break;
		case 1:
			bits_demod[2 * i] = 0; bits_demod[2 * i + 1] = 1;
			break;
		case 2:
			bits_demod[2 * i] = 1; bits_demod[2 * i + 1] = 0;
			break;
		case 3:
			bits_demod[2 * i] = 1; bits_demod[2 * i + 1] = 1;
			break;
		default:
			break;
		}
	}
}

/*16QAM解调*/
void _16QAM_Demodulation_hw(MyComplex* x_hat, int Nt, int* bits_demod)
{
	int i = 0; int j = 0; int best_id = 0;
	MyComplex temp[16];
	MyComplex temp_1[16];
	like_float temp_2;
	like_float distance[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	/*for one in x_hat*/
	for (i = 0; i < Nt; i++)
	{
		for (j = 0; j < 16; j++)
		{
			temp_1[j].real = x_hat[i].real * hls::sqrt((ap_fixed<40,8>)10.0);
			temp_1[j].imag = x_hat[i].imag * hls::sqrt((ap_fixed<40,8>)10.0);
			temp[j].real = temp_1[j].real - _16QAM_Constellation_hw[j].real;
			temp[j].imag = temp_1[j].imag - _16QAM_Constellation_hw[j].imag;
		}
		/*find the closest one*/
		for (j = 0; j < 16; j++)
		{
			temp_2 = (temp[j].real * temp[j].real + temp[j].imag * temp[j].imag);
			distance[j] = hls::sqrt((ap_fixed<40,8>)temp_2);
		}
		best_id = argmin_hw(distance, 16);
		bits_demod[4 * i] = (best_id >> 3) & 0x01;
		bits_demod[4 * i + 1] = (best_id >> 2) & 0x01;
		bits_demod[4 * i + 2] = (best_id >> 1) & 0x01;
		bits_demod[4 * i + 3] = best_id & 0x01;
	}
}

/*64QAM解调*/
void _64QAM_Demodulation_hw(MyComplex* x_hat, int Nt, int* bits_demod)
{
	int i = 0; int j = 0; int best_id = 0;
	MyComplex temp[64];
	MyComplex temp_1[64];
	like_float temp_2;
	like_float distance[64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int int_real; int int_imag;
	/*for one in x_hat*/
	for (i = 0; i < Nt; i++)
	{
		for (j = 0; j < 64; j++)
		{
			temp_1[j].real = x_hat[i].real * hls::sqrt((ap_fixed<40,8>)42.0);
			temp_1[j].imag = x_hat[i].imag * hls::sqrt((ap_fixed<40,8>)42.0);
			temp[j].real = temp_1[j].real - _64QAM_Constellation_hw[j].real;
			temp[j].imag = temp_1[j].imag - _64QAM_Constellation_hw[j].imag;
			distance[j] = temp[j].real * temp[j].real + temp[j].imag * temp[j].imag;
		}
		best_id = argmin_hw(distance, 64);
		switch ((int)_64QAM_Constellation_hw[best_id].real)
		{
		case -7:
			bits_demod[6 * i] = 0; bits_demod[6 * i + 1] = 0; bits_demod[6 * i + 2] = 0;
			break;
		case -5:
			bits_demod[6 * i] = 0; bits_demod[6 * i + 1] = 0; bits_demod[6 * i + 2] = 1;
			break;
		case -3:
			bits_demod[6 * i] = 0; bits_demod[6 * i + 1] = 1; bits_demod[6 * i + 2] = 1;
			break;
		case -1:
			bits_demod[6 * i] = 0; bits_demod[6 * i + 1] = 1; bits_demod[6 * i + 2] = 0;
			break;
		case 1:
			bits_demod[6 * i] = 1; bits_demod[6 * i + 1] = 1; bits_demod[6 * i + 2] = 0;
			break;
		case 3:
			bits_demod[6 * i] = 1; bits_demod[6 * i + 1] = 1; bits_demod[6 * i + 2] = 1;
			break;
		case 5:
			bits_demod[6 * i] = 1; bits_demod[6 * i + 1] = 0; bits_demod[6 * i + 2] = 1;
			break;
		case 7:
			bits_demod[6 * i] = 1; bits_demod[6 * i + 1] = 0; bits_demod[6 * i + 2] = 0;
			break;
		default:
			break;
		}
		switch ((int)_64QAM_Constellation_hw[best_id].imag)
		{
		case -7:
			bits_demod[6 * i + 3] = 0; bits_demod[6 * i + 4] = 0; bits_demod[6 * i + 5] = 0;
			break;
		case -5:
			bits_demod[6 * i + 3] = 0; bits_demod[6 * i + 4] = 0; bits_demod[6 * i + 5] = 1;
			break;
		case -3:
			bits_demod[6 * i + 3] = 0; bits_demod[6 * i + 4] = 1; bits_demod[6 * i + 5] = 1;
			break;
		case -1:
			bits_demod[6 * i + 3] = 0; bits_demod[6 * i + 4] = 1; bits_demod[6 * i + 5] = 0;
			break;
		case 1:
			bits_demod[6 * i + 3] = 1; bits_demod[6 * i + 4] = 1; bits_demod[6 * i + 5] = 0;
			break;
		case 3:
			bits_demod[6 * i + 3] = 1; bits_demod[6 * i + 4] = 1; bits_demod[6 * i + 5] = 1;
			break;
		case 5:
			bits_demod[6 * i + 3] = 1; bits_demod[6 * i + 4] = 0; bits_demod[6 * i + 5] = 1;
			break;
		case 7:
			bits_demod[6 * i + 3] = 1; bits_demod[6 * i + 4] = 0; bits_demod[6 * i + 5] = 0;
			break;
		default:
			break;
		}
	}
}
int argmin_hw(like_float* array, int n)
{
	int i = 0; int best_id = 0;
	like_float minval = array[0];
	for (i = 1; i < n; i++)
	{
		if (minval > array[i])
		{
			minval = array[i];
			best_id = i;
		}
	}
	return best_id;
}
/*计算误bit数*/
int unequal_times_hw(int* array1, int* array2, int n)
{
	int unequal = 0; int i = 0;
	for (i = 0; i < n; i++)
	{
		if (array1[i] != array2[i])
			unequal++;
	}
	return unequal;
}
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/***************************************功能函数***********************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/

// 线性同余生成器
unsigned int lcg_rand_hw() {
    static unsigned int lcg_seed = 1;
	#pragma HLS pipeline II=1
	lcg_seed = (LCG_A * lcg_seed + LCG_C) % LIMIT_MAX;
	
    return lcg_seed;
}
like_float lcg_rand_1_hw() {
    static unsigned int lcg_seed = 1;
	lcg_seed = (LCG_A * lcg_seed + LCG_C) % LIMIT_MAX;
    return (like_float)(lcg_seed / LIMIT_MAX_F);
}
// 复数模长平方（避免开方损失精度）
Myreal complex_norm_sqr(MyComplex a) {
	#pragma HLS INLINE
	return a.real*a.real + a.imag*a.imag;
}
// 复数取共轭
MyComplex complex_conjugate_hw(MyComplex a) {
	#pragma HLS INLINE
	MyComplex result;
	result.real = a.real;   // 实部不变
	result.imag = -a.imag;  // 虚部取负
	return result;
}
template <typename T>
void c_eye_generate_hw(T* Mat, float val)
{
	#pragma HLS INLINE
	like_float val_1 = val;
    int i;
	for (i = 0; i < Ntr_2; i++)
	{
		#pragma HLS unroll
		Mat[i].real = (like_float)0.0;
		Mat[i].imag = (like_float)0.0;
	}
	for (i = 0; i < Ntr_1; i++)
	{
		#pragma HLS unroll
		Mat[i * Ntr_1 + i].real = val;
	}
}
template <typename TH, typename TY, typename TV, typename TH_real, typename TH_imag, typename TY_real, typename TY_imag, typename TV_real, typename TV_imag>
void data_local(TH* H, TY* y, TV* v_tb, TH_real* H_real, TH_imag* H_imag, TY_real* y_real, TY_imag* y_imag, TV_real* v_tb_real, TV_imag* v_tb_imag)
{
	#pragma HLS INLINE off
    int i;
	for(i = 0; i<Ntr_1; i++){
		#pragma HLS unroll
		y[i].real = y_real[i];
		y[i].imag = y_imag[i];
	}
	for (i = 0; i < Ntr_1*Ntr_1; i++)
	{
		#pragma HLS unroll
		H[i].real = H_real[i];
		H[i].imag = H_imag[i];
	}
	for (i = 0; i < Ntr_1*iter_1; i++)
	{
		#pragma HLS unroll
		v_tb[i].real = v_tb_real[i];
		v_tb[i].imag = v_tb_imag[i];
	}
}
template<typename TA, typename TB, typename TR>
void c_matmultiple_hw(TA* matA, int transA, TB* matB, int transB, int ma, int na, int mb, int nb, TR* res)
{
	int m, n, k;
	// 根据转置的情况计算 m, n, k
	if (transA == 0) {  // CblasNoTrans
		m = ma;
		k = na;
	}
	else {
		k = ma;
		m = na;
	}
	if (transB == 0) {  // CblasNoTrans
		n = nb;
	}
	else {
		n = mb;
	}
	// 逐个计算矩阵元素
	for (int i = 0; i < m; i++) {
        #pragma HLS LOOP_TRIPCOUNT max=Ntr_1
		for (int j = 0; j < n; j++) {
            #pragma HLS LOOP_TRIPCOUNT max=Ntr_1
			MyComplex sum =  { (like_float)0.0, (like_float)0.0 }; // 初始化为复数零
			MyComplex temp = { (like_float)0.0, (like_float)0.0 };
			for (int l = 0; l < k; l++) {
				MyComplex a_element, b_element;
				// 获取矩阵A的元素
				if (transA == 1) {  // 共轭转置
					//a_element = matA[l * na + i];  // 获取矩阵A[l, i]
					//a_element = complex_conjugate_hw(a_element);  // 对元素取共轭
					a_element.real = matA[l * na + i].real;
					a_element.imag = -matA[l * na + i].imag;
				}
				else if (transA == 0) {  // 普通转置
					a_element = matA[i * na + l];  // 获取矩阵A[i, l]
				}
				// 获取矩阵B的元素
				if (transB == 0) {  // No Transpose
					// 问题点1-1
					b_element = matB[l * nb + j];  // 获取矩阵B[l, j]
				}
				else {  // 转置
					//b_element = matB[j * nb + l];  // 获取矩阵B[j, l]
					//b_element = complex_conjugate_hw(b_element);  // 对元素取共轭
					b_element.real = matB[j * nb + l].real;
					b_element.imag = -matB[j * nb + l].imag;
				}
				// 复数乘法并累加
				temp = complex_multiply_hw(a_element, b_element);
				sum = complex_add_hw(sum, temp);
			}
			// 将结果赋值到矩阵C（res）
			res[i * nb + j].real = sum.real;
            res[i * nb + j].imag = sum.imag;
		}
	}
}
template<typename TA, typename TB, typename TR>
void my_complex_add_hw(const TA a[], const TB b[], TR r[])
{
	#pragma HLS INLINE
	for (int i = 0; i < Ntr_2; i++) {
		r[i].real = a[i].real + b[i].real; 
		r[i].imag = a[i].imag + b[i].imag;  
	}
}
template<typename TA, typename TB, typename TR>
void my_complex_add_hw_1(const TA a[], const TB b[], TR r[])
{
	#pragma HLS INLINE
	for (int i = 0; i < Ntr_1; i++) {
		r[i].real = a[i].real + b[i].real; 
		r[i].imag = a[i].imag + b[i].imag;  
	}
}
// 初始化复数矩阵
template<typename TA>
void initMatrix_hw(TA* A)
{
	#pragma HLS INLINE
    int i, j;
	for (i = 0; i < Ntr_1; i++)
	{
		#pragma HLS pipeline
		for (j = 0; j < Ntr_1; j++)
		{
			#pragma HLS unroll
			A[i * Ntr_1 + j].real = (like_float)0.0;
			A[i * Ntr_1 + j].imag = (like_float)0.0;
		}
	}
}
// 复数除法
template<typename TA, typename TB>
MyComplex complex_divide_hw(TA a, TB b)
{
	#pragma HLS INLINE
    MyComplex result;
    like_float denominator = b.real * b.real + b.imag * b.imag;
	like_float temp_1 = a.real * b.real + a.imag * b.imag;
	like_float temp_2 = a.imag * b.real - a.real * b.imag;
    result.real = hls_internal::generic_divide((ap_fixed<40,8>)temp_1, (ap_fixed<40,8>)denominator);
    result.imag = hls_internal::generic_divide((ap_fixed<40,8>)temp_2, (ap_fixed<40,8>)denominator);
    return result;
}
// 复数乘法
template<typename TA, typename TB>
MyComplex complex_multiply_hw(TA a, TB b)
{
	#pragma HLS INLINE
    MyComplex result;
	like_float local_temp_1;
	like_float local_temp_2;
	like_float local_temp_3;
	like_float local_temp_4;
    local_temp_1 = a.real * b.real;
    local_temp_2 = a.imag * b.imag;
    local_temp_3 = a.real * b.imag;
    local_temp_4 = a.imag * b.real;
    result.real = local_temp_1 - local_temp_2;
    result.imag = local_temp_3 + local_temp_4;
    return result;
}
// 复数加法
template<typename TA, typename TB>
MyComplex complex_add_hw(TA a, TB b)
{
	#pragma HLS INLINE
    MyComplex result;
    result.real = a.real + b.real;
    result.imag = a.imag + b.imag;
    return result;
}
// 复数减法
template<typename TA, typename TB>
MyComplex complex_subtract_hw(TA a, TB b)
{
	#pragma HLS INLINE
    MyComplex result;
    result.real = a.real - b.real;
    result.imag = a.imag - b.imag;
    return result;
}
// 复数数组减法
template<typename TA, typename TB, typename TR>
void my_complex_sub_hw(const TA a[], const TB b[], TR r[])
{
	#pragma HLS INLINE
	for (int i = 0; i < Ntr_1; i++) {
		#pragma HLS unroll
		r[i].real = a[i].real - b[i].real;  
		r[i].imag = a[i].imag - b[i].imag; 
	}
}
// 矩阵乘法（用于求逆函数中调用）
template<typename TA, typename TB, typename TC>
void MulMatrix_hw(const TA* A, const TB* B, TC* C)
{
	#pragma HLS INLINE
	int i, j, k;
	MyComplex sum, tmp;
	for (i = 0; i < Ntr_1; i++)
	{
		for (j = 0; j < Ntr_1; j++)
		{
			sum.real = 0.0;
			sum.imag = 0.0;
			for (k = 0; k < Ntr_1; k++)
			{
				#pragma HLS unroll II=2
				sum = complex_add_hw(sum, complex_multiply_hw(A[i * Ntr_1 + k], B[k * Ntr_1 + j]));
			}
			C[i * Ntr_1 + j].real = sum.real;
			C[i * Ntr_1 + j].imag = sum.imag;
		}
	}
}
// 复数矩阵求逆
template<typename TA>
void Inverse_LU_hw(TA* A)
{
    MyComplex L[Ntr_2];//row * col
	MyComplex U[Ntr_2];//row * col
	MyComplex L_Inverse[Ntr_2];//row * col
	MyComplex U_Inverse[Ntr_2];//row * col

	// MyComplex* L = &_L[0];
	// MyComplex* U = &_U[0];
	// MyComplex* L_Inverse = &_L_Inverse[0];
	// MyComplex* U_Inverse = &_U_Inverse[0];
    initMatrix_hw(L);
	initMatrix_hw(U);
	initMatrix_hw(L_Inverse);
	initMatrix_hw(U_Inverse);
    int i, j, k, t;
	MyComplex s;
	for (i = 0; i < Ntr_1; i++)//??????
	{
		#pragma HLS pipeline
		L[i * Ntr_1 + i].real = (like_float)1.0;
		L[i * Ntr_1 + i].imag = (like_float)0.0;
	}
	for (j = 0; j < Ntr_1; j++)
	{
		#pragma HLS pipeline
		U[j] = A[j];
	}
	for (i = 1; i < Ntr_1; i++)
	{
		L[i * Ntr_1] = complex_divide_hw(A[i * Ntr_1], U[0]);
	}
	for (k = 1; k < Ntr_1; k++)
	{
		for (j = k; j < Ntr_1; j++)
		{
			s.imag = (like_float)0.0;
			s.real = (like_float)0.0;
			for (t = 0; t < k; t++) {
				s = complex_add_hw(s, complex_multiply_hw(L[k * Ntr_1 + t], U[t * Ntr_1 + j]));
			}
			U[k * Ntr_1 + j] = complex_subtract_hw(A[k * Ntr_1 + j], s);
		}
		for (i = k; i < Ntr_1; i++)
		{
			s.imag = (like_float)0.0;
			s.real = (like_float)0.0;
			for (t = 0; t < k; t++)
			{
				s = complex_add_hw(s, complex_multiply_hw(L[i * Ntr_1 + t], U[t * Ntr_1 + k]));
			}
			L[i * Ntr_1 + k] = complex_divide_hw(complex_subtract_hw(A[i * Ntr_1 + k], s), U[k * Ntr_1 + k]);
		}
	}

	for (i = 0; i < Ntr_1; i++)
	{
		#pragma HLS pipeline
		L_Inverse[i * Ntr_1 + i].imag = (like_float)0.0;
		L_Inverse[i * Ntr_1 + i].real = (like_float)1.0;
	}
	for (j = 0; j < Ntr_1; j++)
	{
		for (i = j + 1; i < Ntr_1; i++)
		{
			s.imag = (like_float)0.0;
			s.real = (like_float)0.0;
			for (k = j; k < i; k++) {
				s = complex_add_hw(s, complex_multiply_hw(L[i * Ntr_1 + k], L_Inverse[k * Ntr_1 + j]));
			}
			L_Inverse[i * Ntr_1 + j].real = (-1) * complex_multiply_hw(L_Inverse[j * Ntr_1 + j], s).real;
			L_Inverse[i * Ntr_1 + j].imag = (-1) * complex_multiply_hw(L_Inverse[j * Ntr_1 + j], s).imag;
		}
	}
	MyComplex di;
	di.real = (like_float)1.0;
	di.imag = (like_float)0.0;
	for (i = 0; i < Ntr_1; i++) //按列序，列内按照从下到上，计算u的逆矩阵
	{
		U_Inverse[i * Ntr_1 + i] = complex_divide_hw(di, U[i * Ntr_1 + i]);

	}
	for (j = 0; j < Ntr_1; j++)
	{
		for (i = j - 1; i >= 0; i--)
		{
			s.imag = (like_float)0.0;
			s.real = (like_float)0.0;
			for (k = i + 1; k <= j; k++)
			{
				s = complex_add_hw(s, complex_multiply_hw(U[i * Ntr_1 + k], U_Inverse[k * Ntr_1 + j]));
			}
			s.imag = -s.imag;
			s.real = -s.real;
			U_Inverse[i * Ntr_1 + j] = complex_divide_hw(s, U[i * Ntr_1 + i]);
		}
	}
	MulMatrix_hw(U_Inverse, L_Inverse, A);
}
// 缩放复数数组
template<typename TX>
void my_complex_scal_hw(const like_float alpha, TX* X, const int incX)
{
	#pragma HLS INLINE
	for (int i = 0; i < Ntr_1; i++) {
		#pragma HLS unroll II=2
		X[i * incX].real *= alpha;  // 实部乘以 alpha
		X[i * incX].imag *= alpha;  // 虚部乘以 alpha
	}
}
template<typename TX>
void my_complex_scal_hw_1(const like_float alpha, TX* X, const int incX)
{
	#pragma HLS INLINE
	for (int i = 0; i < mu_double; i++) {
		#pragma HLS unroll II=2
		X[i * incX].real *= alpha;  // 实部乘以 alpha
		X[i * incX].imag *= alpha;  // 虚部乘以 alpha
	}
}
template <typename T>
T fixed_floor(const T& val) {
	// ap_uint<40> raw_bits = val.range();       // 获取原始二进制表示
    // ap_uint<40> int_mask = 0xFF00000000; // 高32位掩码（保留整数部分）
    // ap_uint<40> int_bits = raw_bits & int_mask;
	const int W = T::width;      // 获取总位宽
    const int I = T::iwidth;     // 获取整数部分位宽（包括符号位）
    
    // 动态生成掩码
    ap_uint<W> int_mask = (ap_uint<W>(~0) << (W - I)); // 整数部分掩码
    ap_uint<W> frac_mask = ~int_mask;                 // 小数部分掩码
    
    ap_uint<W> raw_bits = val.range(); // 获取原始二进制表示
    ap_uint<W> int_bits = raw_bits & int_mask;
    
    // 步骤2：重建截断后的定点数
    // like_float truncated;
    // truncated.range() = int_bits;  // 直接赋整数值部分
	// like_float temp = truncated + 1;
    // truncated = (truncated < 0)?temp:truncated;
    // 步骤3：负值修正逻辑
    // if (val < 0) {
        // 检测是否存在小数部分（比较原始值与截断值）
        // ap_uint<40> frac_mask = 0x00FFFFFFFF; // 低32位掩码（小数部分）
        // bool has_frac = ((raw_bits & frac_mask) != 0);
        // 
        // if (has_frac) {
            // 当存在小数部分时，floor操作需要减1
            // truncated -= like_float(1);
        // }
    // }
	// 重建截断后的定点数
    T truncated;
    truncated.range() = int_bits;
	T temp = truncated + 1;
	truncated = (truncated < 0)?temp:truncated;
    
    // 负值修正逻辑
    if (val < 0) {
        bool has_frac = ((raw_bits & frac_mask) != 0);
        if (has_frac) {
            // 存在小数部分时，floor操作需要减1
            truncated -= T(1);
        }
    }
    return truncated;
}
// 星座点映射
template<typename TX, typename TX_hat, typename x_real, typename hat_real>
void map_hw(like_float dqam, TX* x, TX_hat* x_hat)
{
	#pragma HLS INLINE
    const hat_real one(1.0);
    const like_float divisor = dqam << 1;  // 合并分母运算
	x_real divisor_1 = divisor;
	int i;
	for (i = 0; i < Ntr_1; i++)
	{
		// 实部处理（单次除法+定点floor）
        x_real temp_real = x[i].real / divisor_1;
        x_real floored_real = fixed_floor<x_real>(temp_real);
        x_hat[i].real = (hat_real)((floored_real << 1) + one);

        // 虚部处理（单次除法+定点floor）
        x_real temp_imag = x[i].imag / divisor_1;
        x_real floored_imag = fixed_floor<x_real>(temp_imag);
        x_hat[i].imag = (hat_real)((floored_imag << 1) + one);
		switch (mu_1)
		{
		case 2:
			x_hat[i].real = (x_hat[i].real > hat_real(1.0)) ? hat_real(1.0) : x_hat[i].real;
			x_hat[i].real = (x_hat[i].real < hat_real(-1.0)) ? hat_real(-1.0) : x_hat[i].real;
			x_hat[i].imag = (x_hat[i].imag > hat_real(1.0)) ? hat_real(1.0) : x_hat[i].imag;
			x_hat[i].imag = (x_hat[i].imag < hat_real(-1.0)) ? hat_real(-1.0) : x_hat[i].imag;
			break;
		case 4:
			x_hat[i].real = (x_hat[i].real > hat_real(3.0)) ? hat_real(3.0) : x_hat[i].real;
			x_hat[i].real = (x_hat[i].real < hat_real(-3.0)) ? hat_real(-3.0) : x_hat[i].real;
			x_hat[i].imag = (x_hat[i].imag > hat_real(3.0)) ? hat_real(3.0) : x_hat[i].imag;
			x_hat[i].imag = (x_hat[i].imag < hat_real(-3.0)) ? hat_real(-3.0) : x_hat[i].imag;
			break;
		case 6:
			x_hat[i].real = (x_hat[i].real > hat_real(7.0)) ? hat_real(7.0) : x_hat[i].real;
			x_hat[i].real = (x_hat[i].real < hat_real(-7.0)) ? hat_real(-7.0) : x_hat[i].real;
			x_hat[i].imag = (x_hat[i].imag > hat_real(7.0)) ? hat_real(7.0) : x_hat[i].imag;
			x_hat[i].imag = (x_hat[i].imag < hat_real(-7.0)) ? hat_real(-7.0) : x_hat[i].imag;
			break;
		default:
			break;
		}
	}
	my_complex_scal_hw(dqam, x_hat, 1);
}
// 生成均匀分布随机整数
void generateUniformRandoms_int_hw(int* x_init)
{
	for (int i = 0; i < Ntr_1; ++i) {
		#pragma HLS pipeline II=1
		x_init[i] = (lcg_rand_hw() >> 27) & 0x0F;
	}
}
// 复制复数数组
template<typename TX, typename TY>
void my_complex_copy_hw(const TX* X, const int incX, TY* Y, const int incY)
{
	#pragma HLS inline
	for (int i = 0, j = 0; i < Ntr_1; i++, j++) {
		#pragma HLS pipeline
		Y[j * incY].real = X[i * incX].real;  // 复制实部
		Y[j * incY].imag = X[i * incX].imag;  // 复制虚部
	}
}
// 复制复数数组
template<typename TX, typename TY>
void my_complex_copy_hw_1(const TX* X, const int incX, TY* Y, const int incY)
{
	for (int i = 0, j = 0; i < mu_double; i++, j++) {
		#pragma HLS unroll
		Y[j * incY].real = X[i * incX].real;  // 复制实部
		Y[j * incY].imag = X[i * incX].imag;  // 复制虚部
	}
}
// 生成均匀分布随机变量
void generateUniformRandoms_float_hw(like_float* p_uni)
{//一个随机数生成的函数 其中lcg_rand用于生成随机数，/LIMIT_MAX用于限幅
	for (int i = 0; i < 10; i++) {
		p_uni[i] = lcg_rand_1_hw();
	}
}
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/*********************************顶层函数分步计算函数*******************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/*更新梯度 z_grad = xhat + lr * (grad_preconditioner @ (AH @ r))*/
void z_grad_hw(MyComplex* H, int transA, int transB, MyComplex* temp_Nt, MyComplex* grad_preconditioner, MyComplex* z_grad, like_float lr, MyComplex* x_hat, MyComplex* r)
{
    c_matmultiple_hw_pro(H, transA, r, transB, Ntr_1, Ntr_1, Ntr_1, transA, temp_Nt);
	c_matmultiple_hw_pro(grad_preconditioner, transB, temp_Nt, transB, Ntr_1, Ntr_1, Ntr_1, transA, z_grad);
	my_complex_scal_hw(lr, z_grad, 1); 
	my_complex_add_hw_1(x_hat, z_grad, z_grad);
}
/*加入高斯随机扰动*/
void gauss_add_hw(MyComplex* v, MyComplex* v_tb, int offset, like_float step_size, MyComplex* z_grad, MyComplex* z_prop)
{
    for(int i = 0; i < Ntr_1; i++){
		#pragma HLS pipeline
        v[i].real = v_tb[i+offset].real;
        v[i].imag = v_tb[i+offset].imag;
    }
    //offset = offset + Ntr_1;
    my_complex_scal_hw(step_size, v, 1);
    my_complex_add_hw_1(z_grad, v, z_prop);
}
/*计算新的残差范数 calculate residual norm of the proposal*/
void r_newnorm_hw(MyComplex* H, int transB, MyComplex* x_prop, int transA, MyComplex* temp_Nr, MyComplex* y, MyComplex* r_prop, MyComplex* temp_1, like_float r_norm_prop)
{
    c_matmultiple_hw_pro(H, transB, x_prop, transB, Ntr_1, Ntr_1, Ntr_1, transA, temp_Nr);
	my_complex_sub_hw(y, temp_Nr, r_prop);
	c_matmultiple_hw_pro(r_prop, transA, r_prop, transB, Ntr_1, transA, Ntr_1, transA, temp_1);
	r_norm_prop = temp_1->real;
}
/*update the survivor*/
void survivor_hw(like_float r_norm_survivor, like_float r_norm_prop, MyComplex* x_prop, MyComplex* x_survivor)
{
    if (r_norm_survivor > r_norm_prop)
	{
		my_complex_copy_hw(x_prop, 1, x_survivor, 1);
		r_norm_survivor = r_norm_prop;
	}
}
/*acceptance test＆update GD learning rate＆update random walk size*/
void acceptance_hw(int transB, int transA, like_float r_norm_prop, like_float r_norm, like_float log_pacc, like_float p_acc, like_float* p_uni, MyComplex* x_prop , MyComplex* x_hat, MyComplex* r_prop, MyComplex* r, MyComplex* pmat, MyComplex* pr_prev, MyComplex* temp_1, MyComplex* _temp_1, like_float lr, like_float step_size, like_float dqam, like_float alpha)
{
    like_float temp_3 = -(r_norm_prop - r_norm);
	log_pacc = hls::fmin((ap_fixed<40,8>)0, (ap_fixed<40,8>)temp_3);
	p_acc = hls::exp((ap_fixed<40,8>)log_pacc);
	generateUniformRandoms_float_hw(p_uni);
	like_float step_temp_;
	if (p_acc > p_uni[5])/*概率满足条件时候*/
	{
		my_complex_copy_hw(x_prop, 1, x_hat, 1);
		my_complex_copy_hw(r_prop, 1, r, 1);
		r_norm = r_norm_prop;
		/*update GD learning rate*/
		if (!lr_approx_1)
		{
			c_matmultiple_hw_pro(pmat, transB, r, transB, Ntr_1, Ntr_1, Ntr_1, transA, pr_prev);
			c_matmultiple_hw_pro(r, transA, pr_prev, transB, Ntr_1, transA, Ntr_1, transA, temp_1);
			c_matmultiple_hw_pro(pr_prev, transA, pr_prev, transB, Ntr_1, transA, Ntr_1, transA, _temp_1);
			lr = temp_1->real / _temp_1->real;
		}
		/*update random walk size*/
		step_temp_ = hls_internal::generic_divide((ap_fixed<40,8>)r_norm, (ap_fixed<40,8>)Ntr_1);
		step_size = hls::fmax((ap_fixed<40,8>)dqam, hls::sqrt((ap_fixed<40,8>)step_temp_)) * alpha;
	}
}

/******特殊*******/
void initMatrix(MyComplex_f* A)
{
	int i, j;
	for (i = 0; i < Ntr_1; i++)
	{
		for (j = 0; j < Ntr_1; j++)
		{
			A[i * Ntr_1 + j].real = 0.0;
			A[i * Ntr_1 + j].imag = 0.0;
		}
	}
}
MyComplex_f complex_divide(MyComplex_f a, MyComplex_f b) {
    MyComplex_f result;
    float denominator = b.real * b.real + b.imag * b.imag;
    result.real = (a.real * b.real + a.imag * b.imag) / denominator;
    result.imag = (a.imag * b.real - a.real * b.imag) / denominator;
    return result;
}
MyComplex_f complex_multiply(MyComplex_f a, MyComplex_f b) {
    MyComplex_f result;
    float local_temp_1;
	float local_temp_2;
	float local_temp_3;
	float local_temp_4;
    local_temp_1 = a.real * b.real;
    local_temp_2 = a.imag * b.imag;
    local_temp_3 = a.real * b.imag;
    local_temp_4 = a.imag * b.real;
    result.real = local_temp_1 - local_temp_2;
    result.imag = local_temp_3 + local_temp_4;
    return result;
}
MyComplex_f complex_add(MyComplex_f a, MyComplex_f b) {
    MyComplex_f result;
    result.real = a.real + b.real;
    result.imag = a.imag + b.imag;
    return result;
}
MyComplex_f complex_subtract(MyComplex_f a, MyComplex_f b) {
    MyComplex_f result;
    result.real = a.real - b.real;
    result.imag = a.imag - b.imag;
    return result;
}
void MulMatrix(const MyComplex_f* A, const MyComplex_f* B, MyComplex_f* C)
{
	int i, j, k;
	MyComplex_f sum, tmp;
	for (i = 0; i < Ntr_1; i++)
	{
		for (j = 0; j < Ntr_1; j++)
		{
			sum.real = 0.0;
			sum.imag = 0.0;
			for (k = 0; k < Ntr_1; k++)
			{
				sum = complex_add(sum, complex_multiply(A[i * Ntr_1 + k], B[k * Ntr_1 + j]));
			}
			C[i * Ntr_1 + j].real = sum.real;
			C[i * Ntr_1 + j].imag = sum.imag;
		}
	}
}
void Inverse_LU(MyComplex_f* A)
{
    MyComplex_f L[Ntr_2];//row * col
	MyComplex_f U[Ntr_2];//row * col
	MyComplex_f L_Inverse[Ntr_2];//row * col
	MyComplex_f U_Inverse[Ntr_2];//row * col


	initMatrix(L);
	initMatrix(U);
	initMatrix(L_Inverse);
	initMatrix(U_Inverse);

	int i, j, k, t;
	MyComplex_f s;
	for (i = 0; i < Ntr_1; i++)
	{
		#pragma HLS unroll
		L[i * Ntr_1 + i].real = 1.0;
		L[i * Ntr_1 + i].imag = 0.0;
	}
	for (j = 0; j < Ntr_1; j++)
	{
		#pragma HLS unroll
		U[j] = A[j];
	}
	for (i = 1; i < Ntr_1; i++)
	{
		L[i * Ntr_1] = complex_divide(A[i * Ntr_1], U[0]);
	}
	for (k = 1; k < Ntr_1; k++)
	{
		for (j = k; j < Ntr_1; j++)
		{
			s.imag = 0.0;
			s.real = 0.0;
			for (t = 0; t < k; t++) {
				s = complex_add(s, complex_multiply(L[k * Ntr_1 + t], U[t * Ntr_1 + j]));
			}
			U[k * Ntr_1 + j] = complex_subtract(A[k * Ntr_1 + j], s);
		}
		for (i = k; i < Ntr_1; i++)
		{
			s.imag = 0.0;
			s.real = 0.0;
			for (t = 0; t < k; t++)
			{
				s = complex_add(s, complex_multiply(L[i * Ntr_1 + t], U[t * Ntr_1 + k]));
			}
			L[i * Ntr_1 + k] = complex_divide(complex_subtract(A[i * Ntr_1 + k], s), U[k * Ntr_1 + k]);
		}
	}

	for (i = 0; i < Ntr_1; i++)
	{
		#pragma HLS unroll
		L_Inverse[i * Ntr_1 + i].imag = 0.0;
		L_Inverse[i * Ntr_1 + i].real = 1.0;
	}
	for (j = 0; j < Ntr_1; j++)
	{
		for (i = j + 1; i < Ntr_1; i++)
		{
			s.imag = 0.0;
			s.real = 0.0;
			for (k = j; k < i; k++) {
				s = complex_add(s, complex_multiply(L[i * Ntr_1 + k], L_Inverse[k * Ntr_1 + j]));
			}
			L_Inverse[i * Ntr_1 + j].real = (-1) * complex_multiply(L_Inverse[j * Ntr_1 + j], s).real;
			L_Inverse[i * Ntr_1 + j].imag = (-1) * complex_multiply(L_Inverse[j * Ntr_1 + j], s).imag;
		}
	}
	MyComplex_f di;
	di.real = 1.0;
	di.imag = 0.0;
	for (i = 0; i < Ntr_1; i++)
	{
		U_Inverse[i * Ntr_1 + i] = complex_divide(di, U[i * Ntr_1 + i]);

	}
	for (j = 0; j < Ntr_1; j++)
	{
		for (i = j - 1; i >= 0; i--)
		{
			s.imag = 0.0;
			s.real = 0.0;
			for (k = i + 1; k <= j; k++)
			{
				s = complex_add(s, complex_multiply(U[i * Ntr_1 + k], U_Inverse[k * Ntr_1 + j]));
			}
			s.imag = -s.imag;
			s.real = -s.real;
			U_Inverse[i * Ntr_1 + j] = complex_divide(s, U[i * Ntr_1 + i]);
		}
	}
	MulMatrix(U_Inverse, L_Inverse, A);
}
// 求解上三角系统 R * X = B
void SolveUpperTriangular(MyComplex_f* R, MyComplex_f* B, MyComplex_f* X) {
    for (int j = Ntr_1 - 1; j >= 0; j--) {
        for (int i = 0; i < Ntr_1; i++) {
            MyComplex_f sum;
            sum.real = 0.0;
            sum.imag = 0.0;
            
            // 计算已知部分的累加
            for (int k = j + 1; k < Ntr_1; k++) {
                sum = complex_add(sum, complex_multiply(R[j * Ntr_1 + k], X[k * Ntr_1 + i]));
            }
            
            // 计算当前解
            X[j * Ntr_1 + i] = complex_divide(complex_subtract(B[j * Ntr_1 + i], sum), R[j * Ntr_1 + j]);
        }
    }
}
void Inverse_QR(MyComplex_f* A)
{
    // 工作矩阵
    MyComplex_f Q[Ntr_2];  // 正交矩阵 (m x n)
    MyComplex_f R[Ntr_2];  // 上三角矩阵 (n x n)
    MyComplex_f QH[Ntr_2]; // Q 的共轭转置 (n x m)
    MyComplex_f R_inv[Ntr_2]; // R 的逆 (n x n)
    MyComplex_f temp[Ntr_2]; // 临时存储矩阵

    // 初始化所有矩阵
    initMatrix(Q);
    initMatrix(R);
    initMatrix(QH);
    initMatrix(R_inv);
    initMatrix(temp);
    
    // 步骤 1: 复制 A 到 Q 作为初始值
    for (int i = 0; i < Ntr_2; i++) {
        Q[i] = A[i];
    }

    // 步骤 2: 改进的 Gram-Schmidt 正交化
    for (int k = 0; k < Ntr_1; k++) {
        // 计算当前列的范数 (复数点积)
        MyComplex_f dot_product;
        dot_product.real = 0.0;
        dot_product.imag = 0.0;
        
        for (int i = 0; i < Ntr_1; i++) {
            MyComplex_f conj;
			conj.real = Q[i * Ntr_1 + k].real;
			conj.imag = -Q[i * Ntr_1 + k].imag;
            MyComplex_f prod = complex_multiply(conj, Q[i * Ntr_1 + k]);
            dot_product = complex_add(dot_product, prod);
        }
        
        // 存储 R 的对角元素 (范数)
        R[k * Ntr_1 + k].real = sqrt(dot_product.real); // 范数是实数
        R[k * Ntr_1 + k].imag = 0.0;
        
        // 归一化当前列 (Q的第k列)
        for (int i = 0; i < Ntr_1; i++) {
            Q[i * Ntr_1 + k] = complex_divide(Q[i * Ntr_1 + k], R[k * Ntr_1 + k]);
        }
        
        // 更新后续列
        for (int j = k + 1; j < Ntr_1; j++) {
            // 计算投影系数 <Q_k, Q_j>
            MyComplex_f proj_coeff;
            proj_coeff.real = 0.0;
            proj_coeff.imag = 0.0;
            
            for (int i = 0; i < Ntr_1; i++) {
                MyComplex_f conj;
				conj.real = Q[i * Ntr_1 + k].real;
				conj.imag = -Q[i * Ntr_1 + k].imag;
                MyComplex_f prod = complex_multiply(conj, Q[i * Ntr_1 + j]);
                proj_coeff = complex_add(proj_coeff, prod);
            }
            
            // 存储投影系数到 R
            R[k * Ntr_1 + j] = proj_coeff;
            
            // 从后续列中减去投影
            for (int i = 0; i < Ntr_1; i++) {
                MyComplex_f proj = complex_multiply(Q[i * Ntr_1 + k], proj_coeff);
                Q[i * Ntr_1 + j] = complex_subtract(Q[i * Ntr_1 + j], proj);
            }
        }
    }

    // 步骤 3: 计算 Q 的共轭转置 QH
    for (int i = 0; i < Ntr_1; i++) {
        for (int j = 0; j < Ntr_1; j++) {
            QH[j * Ntr_1 + i].real = Q[i * Ntr_1 + j].real;
			QH[j * Ntr_1 + i].imag = -Q[i * Ntr_1 + j].imag;
        }
    }

    // 步骤 4: 计算 R 的逆 (上三角矩阵求逆)
    // 4.1 初始化 R_inv 为单位矩阵
    initMatrix(R_inv);
    for (int i = 0; i < Ntr_1; i++) {
        R_inv[i * Ntr_1 + i].real = 1.0;
        R_inv[i * Ntr_1 + i].imag = 0.0;
    }
    
    // 4.2 前向替换法求上三角矩阵的逆
    for (int j = 0; j < Ntr_1; j++) {
        // 对角线元素
        R_inv[j * Ntr_1 + j] = complex_divide(R_inv[j * Ntr_1 + j], R[j * Ntr_1 + j]);
        
        // 更新下方元素
        for (int i = j + 1; i < Ntr_1; i++) {
            MyComplex_f sum;
            sum.real = 0.0;
            sum.imag = 0.0;
            
            for (int k = j; k < i; k++) {
                MyComplex_f prod = complex_multiply(R[k * Ntr_1 + i], R_inv[j * Ntr_1 + k]);
                sum = complex_add(sum, prod);
            }
            
            sum.imag = -sum.imag; // 取负
            sum.real = -sum.real;
            
            R_inv[j * Ntr_1 + i] = complex_divide(
                complex_add(sum, R_inv[j * Ntr_1 + i]), 
                R[i * Ntr_1 + i]
            );
        }
    }

    // 步骤 5: 计算 A⁻¹ = R_inv * QH
    MulMatrix(R_inv, QH, temp);
    
    // 步骤 6: 将结果复制回 A
    for (int i = 0; i < Ntr_2; i++) {
        A[i] = temp[i];
    }
}

MyComplex_f complex_sqrt(MyComplex_f a) {
    MyComplex_f res;
    // 仅处理实数平方根（因为Cholesky分解中应为实数）
    res.real = sqrtf(a.real);
    res.imag = 0.0;
    return res;
}

void Inverse_Cholesky(MyComplex_f* A)
{
    MyComplex_f L[Ntr_2];          // Cholesky分解的下三角矩阵
    MyComplex_f Linv[Ntr_2];       // L的逆矩阵
    MyComplex_f LinvH[Ntr_2];      // Linv的共轭转置
    MyComplex_f temp[Ntr_2];       // 临时矩阵

    // 初始化所有矩阵
    initMatrix(L);
    initMatrix(Linv);
    initMatrix(LinvH);
    initMatrix(temp);

    int i, j, k;
	MyComplex_f temp_1, temp_2;
    MyComplex_f sum, conj_val, neg_one, one, zero;
    neg_one.real = -1.0; neg_one.imag = 0.0;
    one.real = 1.0; one.imag = 0.0;
    zero.real = 0.0; zero.imag = 0.0;

    // Cholesky分解: A = L * L^H
    for (j = 0; j < Ntr_1; j++) {
        // 计算对角线元素
        sum = A[j * Ntr_1 + j];
        for (k = 0; k < j; k++) {
            // L(j,k) * conj(L(j,k))
			temp_1.real = L[j * Ntr_1 + k].real;
			temp_1.imag = -L[j * Ntr_1 + k].imag;
            MyComplex_f prod = complex_multiply(L[j * Ntr_1 + k], temp_1);
            sum = complex_subtract(sum, prod);
        }
        // 确保对角元素非负实数
        if (sum.imag != 0.0) {
            // 理论上应为实数，处理数值误差
            sum.imag = 0.0;
        }
        if (sum.real < 0) sum.real = 0; // 避免负实数
        L[j * Ntr_1 + j] = complex_sqrt(sum);

        // 计算非对角线元素
        for (i = j + 1; i < Ntr_1; i++) {
            sum = A[i * Ntr_1 + j];
            for (k = 0; k < j; k++) {
                // L(i,k) * conj(L(j,k))
				temp_2.real = L[j * Ntr_1 + k].real;
				temp_2.imag = -L[j * Ntr_1 + k].imag;
                MyComplex_f prod = complex_multiply(L[i * Ntr_1 + k], temp_2);
                sum = complex_subtract(sum, prod);
            }
            L[i * Ntr_1 + j] = complex_divide(sum, L[j * Ntr_1 + j]);
        }
    }

    // 计算L的逆矩阵 (下三角)
    for (j = 0; j < Ntr_1; j++) {
        // 对角线元素
        Linv[j * Ntr_1 + j] = complex_divide(one, L[j * Ntr_1 + j]);

        // 计算下三角部分
        for (i = j + 1; i < Ntr_1; i++) {
            sum = zero;
            for (k = j; k < i; k++) {
                // L(i,k) * Linv(k,j)
                MyComplex_f prod = complex_multiply(L[i * Ntr_1 + k], Linv[k * Ntr_1 + j]);
                sum = complex_add(sum, prod);
            }
            // Linv(i,j) = -sum / L(i,i)
            MyComplex_f neg_sum = complex_multiply(sum, neg_one);
            Linv[i * Ntr_1 + j] = complex_divide(neg_sum, L[i * Ntr_1 + i]);
        }
    }

    // 计算Linv的共轭转置 (LinvH)
    for (i = 0; i < Ntr_1; i++) {
        for (j = 0; j < Ntr_1; j++) {
			LinvH[i * Ntr_1 + j].real = Linv[j * Ntr_1 + i].real;
			LinvH[i * Ntr_1 + j].imag = -Linv[j * Ntr_1 + i].imag;
        }
    }

    // 计算A⁻¹ = LinvH * Linv
    for (i = 0; i < Ntr_1; i++) {
        for (j = 0; j < Ntr_1; j++) {
            sum = zero;
            for (k = 0; k < Ntr_1; k++) {
                MyComplex_f prod = complex_multiply(LinvH[i * Ntr_1 + k], Linv[k * Ntr_1 + j]);
                sum = complex_add(sum, prod);
            }
            temp[i * Ntr_1 + j] = sum;
        }
    }

    // 确保结果保持Hermitian特性
    for (i = 0; i < Ntr_1; i++) {
        for (j = 0; j <= i; j++) {
            MyComplex_f avg = {
                (temp[i * Ntr_1 + j].real + temp[j * Ntr_1 + i].real) / 2,
                (temp[i * Ntr_1 + j].imag - temp[j * Ntr_1 + i].imag) / 2
            };
            A[i * Ntr_1 + j] = avg;
            A[j * Ntr_1 + i].real = avg.real;
			A[j * Ntr_1 + i].imag = -avg.imag;
        }
    }
}
/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/
/***************************************资源优化型函数****************************************/
/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/
unsigned int fast_mod_barrett(uint64_t a) {
    uint64_t q = (a * BARRETT_MU) >> BARRETT_K; // 近似商
    uint32_t r = a - q * LIMIT_MAX;            // 余数
    if (r >= LIMIT_MAX) r -= LIMIT_MAX;        // 最终调整
    return r;
}
unsigned int lcg_rand_hw_opt(unsigned int &seed) {
    #pragma HLS pipeline II=1
    // 分解运算避免64位溢出（硬件友好）
    uint64_t product = (uint64_t)LCG_A * seed;
    product = fast_mod_barrett(product + LCG_C);
    seed = (unsigned int)product;
    return seed;
}
like_float lcg_rand_1_hw_fixed(unsigned int &seed) {
    #pragma HLS pipeline II=1
    // 生成随机整数（范围：0 ~ 0x7FFFFFFF）
    unsigned int rand_int = lcg_rand_hw_opt(seed);
    // 将31位整数映射到32位小数部分
    ap_uint<64> raw_bits;
    raw_bits.range(63,32) = 0;           // 整数部分清零
    raw_bits.range(31,0)  = rand_int << 1; // 左移1位填充32位小数
    
    // 直接位转换到定点数
    return *reinterpret_cast<like_float*>(&raw_bits);
}
// 生成均匀分布随机整数
void generateUniformRandoms_int_hw_pro_0(unsigned int &seed, int* x_init)
{
	// 生成 Nt 个随机数
	for (int i = 0; i < Ntr_1; ++i) {
		#pragma HLS pipeline II=1
		// 生成0-63
		// x_init[i] = (lcg_rand_hw_opt(seed) >> 25) & 0x3F;

		// 生成 0-15：
		x_init[i] = (lcg_rand_hw_opt(seed) >> 27) & 0x0F; 

		// 生成 0-3：
		// x_init[i] = (lcg_rand_hw_opt(seed) >> 29) & 0x03;
	}
}
void generateUniformRandoms_float_hw_pro(unsigned int &seed, like_float* p_uni)
{//一个随机数生成的函数 其中lcg_rand用于生成随机数，/LIMIT_MAX用于限幅
	for (int i = 0; i < 10; i++) {
		p_uni[i] = lcg_rand_1_hw_fixed(seed);
	}
}

//////矩阵乘法
template<typename TA, typename TB, typename TR>
void c_matmultiple_hw_pro(
    TA* matA, int transA,
    TB* matB, int transB,
    int ma, int na, int mb, int nb,
    TR* res)
{
	#pragma HLS INLINE
	#pragma HLS ARRAY_PARTITION variable=matA complete dim=1
	#pragma HLS ARRAY_PARTITION variable=matB complete dim=1
	#pragma HLS ARRAY_PARTITION variable=res complete dim=1
	(void)ma;
	(void)na;
	(void)mb;
	(void)nb;
	int m = Ntr_1;
	int n = Ntr_1;
	int k = Ntr_1;
	for (int i = 0; i < m; i++) {
		#pragma HLS UNROLL
		for (int j = 0; j < n; j++) {
			#pragma HLS UNROLL
			MyComplex sum =  { (like_float)0.0, (like_float)0.0 }; // 初始化为复数零
			for (int l = 0; l < k; l++) {
				#pragma HLS UNROLL
				TA a_element;
				TB b_element;
				if (transA == 1) {  // 共轭转置
					a_element.real = matA[l * Ntr_1 + i].real;
					a_element.imag = -matA[l * Ntr_1 + i].imag;
				}
				else {
					a_element = matA[i * Ntr_1 + l];  // 获取矩阵A[i, l]
				}
				if (transB == 0) {  // No Transpose
					b_element = matB[l * Ntr_1 + j];  // 获取矩阵B[l, j]
				}
				else {  // 转置
					b_element.real = matB[j * Ntr_1 + l].real;
					b_element.imag = -matB[j * Ntr_1 + l].imag;
				}
				sum = complex_add_hw(sum, complex_multiply_hw(a_element, b_element));
			}
			res[i * Ntr_1 + j].real = sum.real;
            res[i * Ntr_1 + j].imag = sum.imag;
		}
	}
}
// blackbox版本
void c_matmultiple_hw_pro_wrapper(
    ap_fixed<40,8>* matA_real, ap_fixed<40,8>* matA_imag,
    ap_fixed<40,8>* matB_real, ap_fixed<40,8>* matB_imag,
    int transA, int transB,
    int ma, int na, int mb, int nb,
    ap_fixed<40,8>* res_real, ap_fixed<40,8>* res_imag) {
    // 这个函数不会被综合，只是作为黑盒的接口
}

// //脉动阵列版本设计
// template<typename TA, typename TB, typename TR>
// void c_matmultiple_hw_pro_maidong(
//     TA* matA, int transA,
//     TB* matB, int transB,
//     int ma, int na, int mb, int nb,
//     TR* res)
// {
// 	// 确定矩阵维度
//     int m = (transA == 0) ? ma : na;  // 结果矩阵行数
//     int k = (transA == 0) ? na : ma;  // 公共维度
//     int n = (transB == 0) ? nb : mb;  // 结果矩阵列数
// 
// 	// 一些静态int
// 	static int ROW_REG_DEPTH = m + k;
// 	static int COL_REG_DEPTH = n + k;
// 	static int MAX_DEPTH = (ROW_REG_DEPTH > COL_REG_DEPTH) ? ROW_REG_DEPTH : COL_REG_DEPTH;
// 
// 	// 数组声明
// 	TA hang[m][ROW_REG_DEPTH];
// 	TB lie[n][COL_REG_DEPTH];
// 	TA left[m];
// 	TB up[n];
// 	TA right[m][n];
// 	TB down[m][n];
// 	MyComplex sum[m][n];
// 
// 	MyComplex left_in;
// 	MyComplex up_in;
// 
// 	int flag;
// 
// 	MyComplex temp = {0.0, 0.0};
// 	// 行列数组初始化 - 确保未填充区域为0
//     for(int i = 0; i < m; i++) {
//         for(int j = 0; j < ROW_REG_DEPTH; j++) {
// 			#pragma HLS unroll
//             hang[i][j] = {0, 0};  // 确保所有元素初始化为0
//         }
//     }
//     
//     for(int i = 0; i < n; i++) {
//         for(int j = 0; j < COL_REG_DEPTH; j++) {
// 			#pragma HLS unroll
//             lie[i][j] = {0, 0};   // 确保所有元素初始化为0
//         }
//     }
// 	//斜线填充
// 	for(int i=0; i<m; i++){
// 		for(int j=0; j<k; j++){
// 			#pragma HLS pipeline
// 			int r=i+j;
// 			if(r<ROW_REG_DEPTH){
// 				hang[i][r].real = matA[i*k+j].real;
// 				hang[i][r].imag = transA?(-matA[i*k+j].imag):matA[i*k+j].imag;
// 			}
// 		}
// 	}
// 	for(int i=0; i<n; i++){
// 		for(int j=0; j<k; j++){
// 			#pragma HLS pipeline
// 			int r=i+j;
// 			if(r<COL_REG_DEPTH){
// 				lie[i][r].real = matB[j*n+i].real;
// 				lie[i][r].imag = transB?(-matB[j*n+i].imag):matB[j*n+i].imag;
// 			}
// 		}
// 	}
// 
// 	//左侧和上方输入
// 	for(int cycle=0; cycle<MAX_DEPTH; cycle++){
// 		flag = cycle;
// 		for(int i=0; i<m; i++){
// 			left[i].real = (flag < ROW_REG_DEPTH) ? hang[i][flag].real : 0;
// 			left[i].imag = (flag < ROW_REG_DEPTH) ? hang[i][flag].imag : 0;
// 		}
// 		for(int i=0; i<m; i++){
// 			up[i].real = (flag < COL_REG_DEPTH) ? lie[i][flag].real : 0;
// 			up[i].imag = (flag < COL_REG_DEPTH) ? lie[i][flag].imag : 0;
// 		}
// 		for(int row=0; row<m; row++){
// 			for(int col=0; col<n; col++){
// 				#pragma HLS PIPELINE II=1
// 				left_in = (col==0) ? left[row] : right[row][col-1];
// 				up_in = (row==0) ? up[col]: down[row-1][col];
// 				temp = complex_multiply_hw(left_in, up_in);
// 				sum[row][col] = complex_add_hw(sum[row][col], temp);
// 				right[row][col] = left_in;
// 				down[row][col] = up_in;
// 			}
// 		}
// 	}
// 
// 	// 收集结果
//     for (int i = 0; i < m; i++) {
//         for (int j = 0; j < n; j++) {
// 			#pragma HLS unroll
//             res[i * n + j] = sum[i][j];
//         }
//     }
// }
// 
void Inverse_LDL(MyComplex_f* A){
	//过程量定义
	MyComplex_f L[Ntr_2];    // 单位下三角矩阵
    MyComplex_f D[Ntr_1];    // 对角矩阵
    MyComplex_f L_inv[Ntr_2]; // L的逆矩阵
    MyComplex_f D_inv[Ntr_1]; // D的逆矩阵

	//中间量
	MyComplex_f temp_1;
	MyComplex_f temp_2;
	MyComplex_f update_val;
	
	//初始化上述过程矩阵
	for (int i = 0; i < Ntr_1; ++i) {
		#pragma HLS pipeline
        for (int j = 0; j < Ntr_1; ++j) {
            L[i*Ntr_1 + j] = {0.0f, 0.0f};
            L_inv[i*Ntr_1 + j] = {0.0f, 0.0f};
        }
        D[i] = {0.0f, 0.0f};
        L[i*Ntr_1 + i] = {1.0f, 0.0f}; // L对角单位化
    }
	//核心分解算法
	for(int j=0; j < Ntr_1; ++j){
		//计算D
		MyComplex_f sum={0.0f,0.0f};
		for (int k = 0; k < j; ++k) {
            MyComplex_f ljk = L[j*Ntr_1 + k];
			MyComplex_f ljk_H = {ljk.real, -ljk.imag};
			temp_1 = complex_multiply(ljk, ljk_H);
			temp_2 = complex_multiply(temp_1, D[k]);
            sum = complex_add(sum, temp_2);
        }
		D[j] = complex_subtract(A[j*Ntr_1 + j], sum);
		//计算L的列向量
		for (int i = j+1; i < Ntr_1; ++i) {
            MyComplex_f s = {0.0f, 0.0f};
            for (int k = 0; k < j; ++k) {
                MyComplex_f lik = L[i*Ntr_1 + k];
                MyComplex_f ljk = {L[j*Ntr_1 + k].real, -L[j*Ntr_1 + k].imag};
				temp_1 = complex_multiply(lik, ljk);
				temp_2 = complex_multiply(temp_1, D[k]);
                s = complex_add(s, temp_2);
            }
			temp_1 = complex_subtract(A[i*Ntr_1 + j], s);
            L[i*Ntr_1 + j] = complex_divide(temp_1, D[j]);
        }
	}
	// 计算D的逆
    for (int i = 0; i < Ntr_1; ++i) {
		temp_1 = {1.0f, 0.0f};
        D_inv[i] = complex_divide(temp_1, D[i]);
    }
	//计算L的逆（L为单位下三角矩阵）
	for (int i = 0; i < Ntr_1; i++)
	{
		#pragma HLS unroll
		L_inv[i * Ntr_1 + i].imag = 0.0f;
		L_inv[i * Ntr_1 + i].real = 1.0f;
	}
	for (int j = 0; j < Ntr_1; j++)
	{
		for (int i = j + 1; i < Ntr_1; i++)
		{
			temp_1.imag = 0.0;
			temp_1.real = 0.0;
			for (int k = j; k < i; k++) {
				temp_1 = complex_add(temp_1, complex_multiply(L[i * Ntr_1 + k], L_inv[k * Ntr_1 + j]));
			}
			L_inv[i * Ntr_1 + j].real = (-1) * complex_multiply(L_inv[j * Ntr_1 + j], temp_1).real;
			L_inv[i * Ntr_1 + j].imag = (-1) * complex_multiply(L_inv[j * Ntr_1 + j], temp_1).imag;
		}
	}

	//计算最终结果：A⁻¹ = L⁻ᵀ * D⁻¹ * L⁻¹
	MyComplex_f L_inv_T[Ntr_2];
	MyComplex_f temp_3[Ntr_2];
	//1、转置L
	for (int i = 0; i < Ntr_1; ++i) {
        for (int j = 0; j < Ntr_1; ++j) {
            L_inv_T[j*Ntr_1 + i].real = L_inv[i*Ntr_1 + j].real;
			L_inv_T[j*Ntr_1 + i].imag = -L_inv[i*Ntr_1 + j].imag;
        }
    }
	//2、计算D⁻¹ * L⁻¹
	for (int i = 0; i < Ntr_1; ++i) {
        for (int j = 0; j < Ntr_1; ++j) {
            temp_3[i*Ntr_1 + j] = complex_multiply(L_inv[i*Ntr_1 + j], D_inv[i]);
        }
    }
	//3、计算L⁻ᵀ * (D⁻¹ * L⁻¹)
	//MulMatrix(L_inv_T, temp_3, A);
	for (int i = 0; i < Ntr_1; ++i) {
        for (int j = 0; j < Ntr_1; ++j) {
            MyComplex_f sum = {0.0f, 0.0f};
            for (int k = 0; k < Ntr_1; ++k) {
                sum = complex_add(sum, complex_multiply(L_inv_T[i*Ntr_1 + k], temp_3[k*Ntr_1 + j]));
            }
            A[i*Ntr_1 + j] = sum;
        }
    }
}

void Inverse_LDL_pro(MyComplex_f* A){
	//过程量定义
	MyComplex_f L[Ntr_2];    // 单位下三角矩阵
    MyComplex_f D[Ntr_1];    // 对角矩阵
    MyComplex_f L_inv[Ntr_2]; // L的逆矩阵
    MyComplex_f D_inv[Ntr_1]; // D的逆矩阵

	//中间量
	MyComplex_f temp_1;
	MyComplex_f temp_2;
	MyComplex_f update_val;
	MyComplex_f update_val_1;

	//工作矩阵，保留A的下三角部分
	MyComplex_f A_work[Ntr_2];

	//初始化工作矩阵
	for (int i = 0; i < Ntr_1; ++i) {
		#pragma HLS pipeline
        for (int j = 0; j < Ntr_1; ++j) {
            if (i >= j) {  // 下三角部分
                A_work[i*Ntr_1 + j] = A[i*Ntr_1 + j];
            } else {       // 上三角部分清零
                A_work[i*Ntr_1 + j] = (MyComplex_f){0.0f, 0.0f};
            }
        }
    }
	
	//初始化上述过程矩阵
	for (int i = 0; i < Ntr_1; ++i) {
		#pragma HLS pipeline
        for (int j = 0; j < Ntr_1; ++j) {
            L[i*Ntr_1 + j] = {0.0f, 0.0f};
            L_inv[i*Ntr_1 + j] = {0.0f, 0.0f};
        }
        D[i] = {0.0f, 0.0f};
        L[i*Ntr_1 + i] = {1.0f, 0.0f}; // L对角单位化
    }
	//核心分解算法
	for(int j=0; j < Ntr_1; ++j){
		//计算D——————>直接使用变换后的对角元素
		D[j] = A_work[j*Ntr_1 + j];
		//计算L的第j列
		for (int i = j + 1; i < Ntr_1; ++i) {
            L[i*Ntr_1 + j] = complex_divide(A_work[i*Ntr_1 + j], D[j]);
        }

		//子矩阵更新（Right-Looking）
		if (j < Ntr_1 - 1) {
            for (int i = j + 1; i < Ntr_1; ++i) {
				update_val_1 = complex_multiply(L[i*Ntr_1 + j], D[j]);
                for (int j_prime = j + 1; j_prime <= i; ++j_prime) {
                    // 计算更新量: L(i,j)*D(j)*conj(L(j_prime,j))
                    MyComplex_f L_jprime_j_conj = {L[j_prime*Ntr_1 + j].real, -L[j_prime*Ntr_1 + j].imag};
                    update_val = complex_multiply(update_val_1, L_jprime_j_conj);
                    // 更新工作矩阵的下三角部分
                    A_work[i*Ntr_1 + j_prime] = complex_subtract(A_work[i*Ntr_1 + j_prime], update_val);
                }
            }
        }
	}
	// 计算D的逆
    for (int i = 0; i < Ntr_1; ++i) {
		temp_1 = {1.0f, 0.0f};
        D_inv[i] = complex_divide(temp_1, D[i]);
    }
	//计算L的逆（L为单位下三角矩阵）
	for (int i = 0; i < Ntr_1; i++)
	{
		#pragma HLS unroll
		L_inv[i * Ntr_1 + i].imag = 0.0f;
		L_inv[i * Ntr_1 + i].real = 1.0f;
	}
	for (int j = 0; j < Ntr_1; j++)
	{
		for (int i = j + 1; i < Ntr_1; i++)
		{
			temp_1.imag = 0.0;
			temp_1.real = 0.0;
			for (int k = j; k < i; k++) {
				temp_1 = complex_add(temp_1, complex_multiply(L[i * Ntr_1 + k], L_inv[k * Ntr_1 + j]));
			}
			L_inv[i * Ntr_1 + j].real = (-1) * complex_multiply(L_inv[j * Ntr_1 + j], temp_1).real;
			L_inv[i * Ntr_1 + j].imag = (-1) * complex_multiply(L_inv[j * Ntr_1 + j], temp_1).imag;
		}
	}

	//计算最终结果：A⁻¹ = L⁻ᵀ * D⁻¹ * L⁻¹
	MyComplex_f L_inv_T[Ntr_2];
	MyComplex_f temp_3[Ntr_2];
	//1、转置L
	for (int i = 0; i < Ntr_1; ++i) {
        for (int j = 0; j < Ntr_1; ++j) {
            L_inv_T[j*Ntr_1 + i].real = L_inv[i*Ntr_1 + j].real;
			L_inv_T[j*Ntr_1 + i].imag = -L_inv[i*Ntr_1 + j].imag;
        }
    }
	//2、计算D⁻¹ * L⁻¹
	for (int i = 0; i < Ntr_1; ++i) {
        for (int j = 0; j < Ntr_1; ++j) {
            temp_3[i*Ntr_1 + j] = complex_multiply(L_inv[i*Ntr_1 + j], D_inv[i]);
        }
    }
	//3、计算L⁻ᵀ * (D⁻¹ * L⁻¹)
	//MulMatrix(L_inv_T, temp_3, A);
	for (int i = 0; i < Ntr_1; ++i) {
        for (int j = 0; j < Ntr_1; ++j) {
			#pragma HLS pipeline
            MyComplex_f sum = {0.0f, 0.0f};
            for (int k = 0; k < Ntr_1; ++k) {
                sum = complex_add(sum, complex_multiply(L_inv_T[i*Ntr_1 + k], temp_3[k*Ntr_1 + j]));
            }
            A[i*Ntr_1 + j] = sum;
        }
    }
}
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/************************************dataflow函数**********************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
void get_dqam_hw(like_float &dqam){
	#pragma HLS INLINE off
    like_float numerator = 1.5;
    like_float denominator = hls::pow((ap_fixed<40,8>)2, (ap_fixed<40,8>)mu_1) - (ap_fixed<40,8>)1;
	like_float tttt = numerator/denominator;
	dqam = hls::sqrt((ap_fixed<40,8>)tttt);
}
void constellation_norm_initial(
	MyComplex* constellation_norm, like_float dqam
){
	my_complex_copy_hw_1<MyComplex, MyComplex>(_16QAM_Constellation_hw, 1, constellation_norm, 1);
	my_complex_scal_hw_1<MyComplex>(dqam, constellation_norm, 1);
}
void grad_preconditioner_updater_hw(
	MyComplex_H* H, MyComplex_HH* HH_H, MyComplex_sigma2eye* sigma2eye, 
	MyComplex_grad_preconditioner* grad_preconditioner, float sigma2_local, like_float dqam
){
	int transA = 1;  // CblasConjTrans 的等效值，表示共轭转置
	int transB = 0;  // CblasNoTrans 的等效值，表示不转置
	int i;
	MyComplex_f local_f_complex_2[Ntr_2]; 
	float local_1;
	float local_2;
    c_eye_generate_hw<MyComplex_sigma2eye>(sigma2eye, sigma2_local / (float)(dqam * dqam));
	c_matmultiple_hw_pro<MyComplex_H, MyComplex_H, MyComplex_HH>(H, transA, H, transB, Ntr_1, Ntr_1, Ntr_1, Ntr_1, HH_H);
    my_complex_add_hw<MyComplex_HH, MyComplex_sigma2eye, MyComplex_grad_preconditioner>(HH_H, sigma2eye, grad_preconditioner);
	for(i=0; i<Ntr_2; i++){
		local_f_complex_2[i].real = grad_preconditioner[i].real;
		local_f_complex_2[i].imag = grad_preconditioner[i].imag;
	}
	for(i=0; i<Ntr_2; i++){
		local_1 = fabsf(local_f_complex_2[i].real);
        local_2 = fabsf(local_f_complex_2[i].imag);
		local_f_complex_2[i].real = (local_1 < 0.000001f)?0.0f:local_f_complex_2[i].real;
		local_f_complex_2[i].imag = (local_2 < 0.000001f)?0.0f:local_f_complex_2[i].imag;
	}
    Inverse_LDL_pro(local_f_complex_2);//Inverse_LU(local_f_complex_2);
	for(i=0; i<Ntr_2; i++){
		grad_preconditioner[i].real = local_f_complex_2[i].real;
		grad_preconditioner[i].imag = local_f_complex_2[i].imag;
	}
}
void get_alpha(like_float &alpha){
	like_float exponent = like_float(1) / like_float(3); // 避免浮点字面值隐式转换
	like_float tttt_pppp = hls::divide<40,8>((ap_fixed<40,8>)Ntr_1, (ap_fixed<40,8>)8);
	// like_float exponent_1 = (like_float)Ntr_1 / like_float(8);
    alpha = like_float(1) / hls::pow<40,8>((ap_fixed<40,8>)tttt_pppp, (ap_fixed<40,8>)exponent);
}
/*For learning rate line search*/
template<typename TH, typename T_grad, typename T_pat>
void learning_rate_line_search_hw(int lr_approx, TH* H, T_grad* grad_preconditioner, int Nr, int Nt, T_pat* pmat)
{
	int transA = 1;  // CblasConjTrans 的等效值，表示共轭转置
	int transB = 0;  // CblasNoTrans 的等效值，表示不转置
	MyComplex_temp_NtNr temp_NtNr[Ntr_2];
    if (!lr_approx)
	{
		c_matmultiple_hw_pro(H, transB, grad_preconditioner, transB, Nt, Nt, Nt, Nt, temp_NtNr);
		c_matmultiple_hw_pro(temp_NtNr, transB, H, transA, Nr, Nt, Nr, Nt, pmat);
	}else
	{
		 for (int i = 0; i < Ntr_2; i++)
		{
			#pragma HLS unroll
			pmat[i].real = (like_float)0; 
			pmat[i].imag = (like_float)0;
		}
	}
}
void x_initialize_hw(
	int mmse_init, MyComplex_sigma2eye* sigma2eye, int Nt, int Nr, float sigma2, MyComplex_HH* HH_H, 
	MyComplex_H* H, MyComplex_y* y, int num,
	like_float dqam, MyComplex* x_hat, MyComplex* constellation_norm, unsigned int& seed
){
	int i;
	int transA = 1;  // CblasConjTrans 的等效值，表示共轭转置
	int transB = 0;  // CblasNoTrans 的等效值，表示不转置
	MyComplex_f local_f_complex_2[Ntr_2]; 
	MyComplex_temp_NtNt temp_NtNt[Ntr_2];
	MyComplex_x_mmse x_mmse[Ntr_1];
	float local_1;
	float local_2;
	MyComplex_temp_Nt temp_Nt[Ntr_1];
	int x_init_1[Ntr_1]={1, 11, 3, 6, 13, 11, 15, 9};
    if (mmse_init_1)
	{
		/*x_mmse = la.inv(AHA + noise_var * np.eye(nt)) @ AH @ y*/
		c_eye_generate_hw<MyComplex_sigma2eye>(sigma2eye, sigma2);
		my_complex_add_hw<MyComplex_sigma2eye, MyComplex_HH, MyComplex_temp_NtNt>(sigma2eye, HH_H, temp_NtNt);
		for(i=0; i<Ntr_2; i++){
			local_f_complex_2[i].real = temp_NtNt[i].real;
			local_f_complex_2[i].imag = temp_NtNt[i].imag;
		}
		for(i=0; i<Ntr_2; i++){
			local_1 = fabsf(local_f_complex_2[i].real);
			local_2 = fabsf(local_f_complex_2[i].imag);
			local_f_complex_2[i].real = (local_1 < 0.000001f)?0.0f:local_f_complex_2[i].real;
			local_f_complex_2[i].imag = (local_2 < 0.000001f)?0.0f:local_f_complex_2[i].imag;
		}
		Inverse_LDL_pro(local_f_complex_2);//Inverse_LU(local_f_complex_2);
		for(i=0; i<Ntr_2; i++){
			temp_NtNt[i].real = local_f_complex_2[i].real;
			temp_NtNt[i].imag = local_f_complex_2[i].imag;
		}
		c_matmultiple_hw_pro<MyComplex_H, MyComplex_y, MyComplex_temp_Nt>(H, transA, y, transB, Ntr_1, Ntr_1, Ntr_1, transA, temp_Nt);
		c_matmultiple_hw_pro<MyComplex_temp_NtNt, MyComplex_temp_Nt, MyComplex_x_mmse>(temp_NtNt, transB, temp_Nt, transB, Ntr_1, Ntr_1, Ntr_1, transA, x_mmse);
		/*映射到归一化星座图中 xhat = constellation_norm[np.argmin(abs(x_mmse * np.ones(nt, 2 * *mu) - constellation_norm), axis = 1)].reshape(-1, 1)*/
		map_hw<MyComplex_x_mmse, MyComplex, x_mmse_real_t, Myreal>(dqam, x_mmse, x_hat);
	}else
	{
	/*xhat = constellation_norm[np.random.randint(low=0, high=2 ** mu, size=(samplers, nt, 1))].copy()*/
	generateUniformRandoms_int_hw_pro_0(seed, x_init_1);
	for (int i = 0; i < Ntr_1; i++)
		x_hat[i] = constellation_norm[x_init_1[i]];
	}
}
/*计算剩余向量r=y-Hx*/
void r_hw(MyComplex_H* H, MyComplex* x_hat, MyComplex_r* r, MyComplex_y* y)
{
	MyComplex_r r_local[Ntr_1];
	for(int i=0; i<Ntr_1; i++){
		r_local[i].real = r[i].real;
		r_local[i].imag = r[i].imag;
	}
	int transA = 1;  // CblasConjTrans 的等效值，表示共轭转置
	int transB = 0;  // CblasNoTrans 的等效值，表示不转置
    c_matmultiple_hw_pro<MyComplex_H, MyComplex, MyComplex_r>(H, transB, x_hat, transB, Ntr_1, Ntr_1, Ntr_1, transA, r_local);
	my_complex_sub_hw<MyComplex_y, MyComplex_r, MyComplex_r>(y, r_local, r_local);
	for(int i=0; i<Ntr_1; i++){
		r[i].real = r_local[i].real;
		r[i].imag = r_local[i].imag;
	}
}
void r_cal_hw(MyComplex_r* r, MyComplex* x_hat, MyComplex* x_survivor, r_norm_t &r_norm, r_norm_t &r_norm_survivor)
{
	int transA = 1;  // CblasConjTrans 的等效值，表示共轭转置
	int transB = 0;  // CblasNoTrans 的等效值，表示不转置
	MyComplex_temp_1 temp_1[1];
    c_matmultiple_hw_pro<MyComplex_r, MyComplex_r, MyComplex_temp_1>(r, transA, r, transB, Ntr_1, transA, Ntr_1, transA, temp_1);
	r_norm = temp_1[0].real;
	my_complex_copy_hw<MyComplex, MyComplex>(x_hat, 1, x_survivor, 1);
	r_norm_survivor = r_norm;
}
void lr_hw(int lr_approx, MyComplex_pmat* pmat, MyComplex_r* r, MyComplex_pr_prev* pr_prev, lr_t &lr, int num)
{
	MyComplex__temp_1 _temp_1[1];
	MyComplex_temp_1 temp_1[1];
	local_temp_1_t local_temp_1;
	local_temp_2_t local_temp_2;
    MyComplex_pr_prev pr_prev_local[Ntr_1];
	int transA = 1;  // CblasConjTrans 的等效值，表示共轭转置
	int transB = 0;  // CblasNoTrans 的等效值，表示不转置
	if (!lr_approx)
	{
		c_matmultiple_hw_pro<MyComplex_pmat, MyComplex_r, MyComplex_pr_prev>(pmat, transB, r, transB, Ntr_1, Ntr_1, Ntr_1, transA, pr_prev_local);
        for(int i=0; i<Ntr_1; i++){
            pr_prev[i].real = pr_prev_local[i].real;
            pr_prev[i].imag = pr_prev_local[i].imag;
        }        
		c_matmultiple_hw_pro<MyComplex_r, MyComplex_pr_prev, MyComplex_temp_1>(r, transA, pr_prev, transB, Ntr_1, transA, Ntr_1, transA, temp_1);
		c_matmultiple_hw_pro<MyComplex_pr_prev, MyComplex_pr_prev, MyComplex__temp_1>(pr_prev, transA, pr_prev, transB, Ntr_1, transA, Ntr_1, transA, _temp_1);
		local_temp_1 = temp_1[0].real;
		local_temp_2 = _temp_1[0].real;
		lr = hls_internal::generic_divide((local_temp_1_t)local_temp_1, (local_temp_2_t)local_temp_2);
	}else
	{
		lr = num * 0.5;
	}
}
void step_size_hw(step_size_t &step_size, like_float alpha, like_float dqam, r_norm_t r_norm){
	local_temp_1_t local_temp_3;
	local_temp_2_t local_temp_4;
	local_temp_3 = hls_internal::generic_divide((r_norm_t)r_norm, (r_norm_t)Ntr_1);
	local_temp_4 = hls::sqrt((local_temp_1_t)local_temp_3);
    step_size = hls::fmax((ap_fixed<40,8>)dqam, (ap_fixed<40,8>)local_temp_4) * alpha;
}
void data_copy(
	/*静态量*/
	MyComplex_H H_local[Ntr_2], MyComplex_y y_local[Ntr_1], MyComplex_v v_tb_local[Ntr_1 * iter_1], MyComplex_grad_preconditioner grad_preconditioner[Ntr_2],
	MyComplex_pmat pmat[Ntr_2], MyComplex constellation_norm[mu_double], like_float dqam, like_float alpha, MyComplex_sigma2eye sigma2eye[Ntr_2], MyComplex_HH HH_H[Ntr_2],
	/*copy结果*/
	MyComplex_H* H_local_2, MyComplex_y* y_local_2, MyComplex_v* v_tb_local_2, MyComplex_grad_preconditioner* grad_preconditioner_2,
	MyComplex_pmat* pmat_2, MyComplex* constellation_norm_2, like_float &dqam_2, like_float &alpha_2, MyComplex_sigma2eye* sigma2eye_2, MyComplex_HH* HH_H_2
){
	int i, j, k, l;
	dqam_2 = dqam;
	alpha_2 = alpha;
	for(i=0; i<Ntr_2; ++i){
		H_local_2[i].real = H_local[i].real;
		H_local_2[i].imag = H_local[i].imag;
		grad_preconditioner_2[i].real = grad_preconditioner[i].real;
		grad_preconditioner_2[i].imag = grad_preconditioner[i].imag;
		pmat_2[i].real = pmat[i].real;
		pmat_2[i].imag = pmat[i].imag;
		sigma2eye_2[i].real = sigma2eye[i].real;
		sigma2eye_2[i].imag = sigma2eye[i].imag;
		HH_H_2[i].real = HH_H[i].real;
		HH_H_2[i].imag = HH_H[i].imag;
	}
	for(j=0; j<Ntr_1; ++j){
		y_local_2[j].real = y_local[j].real;
		y_local_2[j].imag = y_local[j].imag;
	}
	for(k=0; k<Ntr_1 * iter_1; ++k){
		v_tb_local_2[k].real = v_tb_local[k].real;
		v_tb_local_2[k].imag = v_tb_local[k].imag;
	}
	for(l=0; l<mu_double; ++l){
		constellation_norm_2[l].real = constellation_norm[l].real;
		constellation_norm_2[l].imag = constellation_norm[l].imag;
	}
}
void samplers_process(
	/*静态量*/
	MyComplex_H H_local[Ntr_2], MyComplex_y y_local[Ntr_1], MyComplex_v v_tb_local[Ntr_1 * iter_1], MyComplex_grad_preconditioner grad_preconditioner[Ntr_2],
	MyComplex_pmat pmat[Ntr_2], MyComplex constellation_norm[mu_double], like_float dqam, like_float alpha, float sigma2_local, int lr_approx, int num,
	/*动态*/
	MyComplex* x_hat, MyComplex_r* r, r_norm_t &r_norm, MyComplex_pr_prev* pr_prev, lr_t &lr,
	step_size_t &step_size, int &offset, MyComplex* x_survivor, r_norm_t &r_norm_survivor, unsigned int& seed
){
	#pragma HLS INLINE off
	/*局部变量*/
	MyComplex_v v[Ntr_1];
	MyComplex_z_grad z_grad[Ntr_1];
	MyComplex_z_prop z_prop[Ntr_1];
	MyComplex_x_prop x_prop[Ntr_1];
	MyComplex_r r_prop[Ntr_1];
	MyComplex_temp_Nt temp_Nt[Ntr_1];
	MyComplex_temp_Nr temp_Nr[Ntr_1];
	MyComplex_temp_1 temp_1[1];
	MyComplex__temp_1 _temp_1[1];
	like_float log_pacc;
	like_float p_acc;
	like_float delta_norm;
	like_float p_uni[10];// = {0.0546594001, 0.372195959, 0.999145865, 0.0510859713, 0.411008626, 0.0656750798, 0.0993093923, 0.258695126, 0.443532109, 0.960875571};
	r_norm_t r_norm_prop;
	local_temp_1_t local_temp_1;
	local_temp_2_t local_temp_2;
	int transA = 1;  // CblasConjTrans 的等效值，表示共轭转置
	int transB = 0;  // CblasNoTrans 的等效值，表示不转置
	
	for (int k = 0; k < iter_1; k++){
		#pragma HLS PIPELINE II=1
		/*更新梯度 z_grad = xhat + lr * (grad_preconditioner @ (AH @ r))*/
    	//z_grad_hw(H_local, transA, transB, temp_Nt, grad_preconditioner, z_grad, lr, x_hat_1, r);
		c_matmultiple_hw_pro<MyComplex_H, MyComplex_r, MyComplex_temp_Nt>(H_local, transA, r , transB, Ntr_1, Ntr_1, Ntr_1, transA, temp_Nt);
		// c_matmultiple_hw_pro_wrapper(H_local.real, H_local.imag, r.real, r.imag, transA, transB, Ntr_1, Ntr_1, Ntr_1, transA, temp_Nt.real, temp_Nt.imag);
		c_matmultiple_hw_pro<MyComplex_grad_preconditioner, MyComplex_temp_Nt, MyComplex_z_grad>(grad_preconditioner, transB, temp_Nt, transB, Ntr_1, Ntr_1, Ntr_1, transA, z_grad);
		my_complex_scal_hw<MyComplex_z_grad>(lr, z_grad, 1); 
		my_complex_add_hw_1<MyComplex, MyComplex_z_grad, MyComplex_z_grad>(x_hat, z_grad, z_grad);
    	/*加入高斯随机扰动*/
    	///gauss_add_hw(v, v_tb_local, offset, step_size, z_grad, z_prop);
		for(int i = 0; i < Ntr_1; i++){
			v[i].real = v_tb_local[i+offset].real;
			v[i].imag = v_tb_local[i+offset].imag;
		}
		offset = (offset>(num_ran-Ntr_1))?0:(offset + Ntr_1);
		// c_matmultiple_hw_pro<MyComplex, MyComplex_v, MyComplex_v>(covar, transB, v , transB, Ntr_1, Ntr_1, Ntr_1, transA, v);
		my_complex_scal_hw<MyComplex_v>(step_size, v, 1);
		my_complex_add_hw_1<MyComplex_z_grad, MyComplex_v, MyComplex_z_prop>(z_grad, v, z_prop);
    	/*将梯度映射到QAM星座点中 x_prop = constellation_norm[np.argmin(abs(z_prop * ones - constellation_norm), axis=2)].reshape(-1, nt, 1) */
    	map_hw<MyComplex_z_prop, MyComplex_x_prop, z_prop_real_t, x_prop_real_t>(dqam, z_prop, x_prop);
    	/*计算新的残差范数 calculate residual norm of the proposal*/
    	//r_newnorm_hw(H_local, transB, x_prop, transA, temp_Nr, y_local, r_prop, temp_1, r_norm_prop);
		c_matmultiple_hw_pro<MyComplex_H, MyComplex_x_prop, MyComplex_temp_Nr>(H_local, transB, x_prop, transB, Ntr_1, Ntr_1, Ntr_1, transA, temp_Nr);
		my_complex_sub_hw<MyComplex_y, MyComplex_temp_Nr, MyComplex_r>(y_local, temp_Nr, r_prop);
		c_matmultiple_hw_pro<MyComplex_r, MyComplex_r, MyComplex_temp_1>(r_prop, transA, r_prop, transB, Ntr_1, transA, Ntr_1, transA, temp_1);
		r_norm_prop = temp_1[0].real;
    	/*update the survivor*/
    	//survivor_hw(r_norm_survivor, r_norm_prop, x_prop, x_survivor);
		if (r_norm_survivor > r_norm_prop)
		{
			my_complex_copy_hw<MyComplex_x_prop, MyComplex>(x_prop, 1, x_survivor, 1);
			r_norm_survivor = r_norm_prop;
		}
    	/*acceptance test＆update GD learning rate＆update random walk size*/
    	//acceptance_hw(transB, transA, r_norm_prop, r_norm, log_pacc, p_acc, p_uni, x_prop , x_hat_1, r_prop, r, pmat, pr_prev, &temp_1, &_temp_1, lr, step_size, dqam, alpha);
    	local_temp_1 = (r_norm - r_norm_prop);// * (local_temp_1_t)5;
		log_pacc = hls::fmin((local_temp_1_t)0, (local_temp_1_t)local_temp_1);
		p_acc = hls::exp((ap_fixed<40,8>)log_pacc);
		/*接收判定修改*/
		// delta_norm = -(r_norm_prop - r_norm);
		// p_acc = delta_norm * (like_float)0.25;
		// //防止浮点下溢导致 p_acc=0
		// if (p_acc > 20) {  // exp(-20) ≈ 2e-9，视为0
		    // p_acc = 0;
		// } else {
		    // p_acc = hls::exp(p_acc);
		// }
		generateUniformRandoms_float_hw_pro(seed, p_uni);
		if (p_acc > p_uni[5])/*概率满足条件时候*/
		{
			my_complex_copy_hw<MyComplex_x_prop, MyComplex>(x_prop, 1, x_hat, 1);
			my_complex_copy_hw<MyComplex_r, MyComplex_r>(r_prop, 1, r, 1);
			r_norm = r_norm_prop;
			/*update GD learning rate*/
			if (!lr_approx)
			{
				c_matmultiple_hw_pro<MyComplex_pmat, MyComplex_r, MyComplex_pr_prev>(pmat, transB, r, transB, Ntr_1, Ntr_1, Ntr_1, transA, pr_prev);
				c_matmultiple_hw_pro<MyComplex_r, MyComplex_pr_prev, MyComplex_temp_1>(r, transA, pr_prev, transB, Ntr_1, transA, Ntr_1, transA, temp_1);
				c_matmultiple_hw_pro<MyComplex_pr_prev, MyComplex_pr_prev, MyComplex__temp_1>(pr_prev, transA, pr_prev, transB, Ntr_1, transA, Ntr_1, transA, _temp_1);
				lr = temp_1[0].real / _temp_1[0].real;
			}
			/*update random walk size*/
			local_temp_1 = hls_internal::generic_divide((ap_fixed<40,8>)r_norm, (ap_fixed<40,8>)Ntr_1);
			local_temp_2 = hls::fmax((ap_fixed<40,8>)dqam, hls::sqrt((ap_fixed<40,8>)local_temp_1));
			step_size = local_temp_2 * alpha;
		}
	}
}
void comparison_r(
	/*静态量*/
	r_norm_t r_norm_survivor_1, r_norm_t r_norm_survivor_2, 
	r_norm_t r_norm_survivor_3, r_norm_t r_norm_survivor_4,
	// r_norm_t r_norm_survivor_5, r_norm_t r_norm_survivor_6,
	// r_norm_t r_norm_survivor_7, r_norm_t r_norm_survivor_8,
	MyComplex x_survivor[Ntr_1], MyComplex x_survivor_2[Ntr_1], 
	MyComplex x_survivor_3[Ntr_1], MyComplex x_survivor_4[Ntr_1],
	// MyComplex x_survivor_5[Ntr_1], MyComplex x_survivor_6[Ntr_1],
	// MyComplex x_survivor_7[Ntr_1], MyComplex x_survivor_8[Ntr_1],
	/*结果量*/
	MyComplex* x_survivor_final
){
	int choice;
	int i;
	r_norm_t r_norm_survivor_all[samplers];
	r_norm_t r_norm_survivor_final;
	/*比较不同采样器结果（即比较r_norm_survivor大小）*/
	r_norm_survivor_final = r_norm_survivor_1;
	r_norm_survivor_all[0] = r_norm_survivor_1;
	r_norm_survivor_all[1] = r_norm_survivor_2;
	r_norm_survivor_all[2] = r_norm_survivor_3;
	r_norm_survivor_all[3] = r_norm_survivor_4;

	// r_norm_survivor_all[4] = r_norm_survivor_5;
	// r_norm_survivor_all[5] = r_norm_survivor_6;
	// r_norm_survivor_all[6] = r_norm_survivor_7;
	// r_norm_survivor_all[7] = r_norm_survivor_8;
	for(i=0; i<samplers; ++i){
		if(r_norm_survivor_all[i] < r_norm_survivor_final){
			choice = i+1;
		}
	}
	/*选择x_survivor*/
	switch(choice){
		case 1:
			for(i=0; i<Ntr_1; ++i)x_survivor_final[i] = x_survivor[i];
			break;
		case 2:
			for(i=0; i<Ntr_1; ++i)x_survivor_final[i] = x_survivor_2[i];
			break;
		case 3:
			for(i=0; i<Ntr_1; ++i)x_survivor_final[i] = x_survivor_3[i];
			break;
		case 4:
			for(i=0; i<Ntr_1; ++i)x_survivor_final[i] = x_survivor_4[i];
			break;
		// case 5:
			// for(i=0; i<Ntr_1; ++i)x_survivor_final[i] = x_survivor_5[i];
			// break;
		// case 6:
			// for(i=0; i<Ntr_1; ++i)x_survivor_final[i] = x_survivor_6[i];
			// break;
		// case 7:
			// for(i=0; i<Ntr_1; ++i)x_survivor_final[i] = x_survivor_7[i];
			// break;
		// case 8:
			// for(i=0; i<Ntr_1; ++i)x_survivor_final[i] = x_survivor_8[i];
			// break;
		default:
			for(i=0; i<Ntr_1; ++i)x_survivor_final[i] = x_survivor[i];
			break;
	}
}
template<typename TX, typename TY_real, typename TY_imag>
void out_hw(const TX* X, const int incX, TY_real* Y_real, TY_imag* Y_imag, int incY)
{
	for (int i = 0, j = 0; i < Ntr_1; i++, j++) {
		Y_real[j * incY] = X[i * incX].real;  // 复制实部
		Y_imag[j * incY] = X[i * incX].imag;  // 复制虚部
	}
}
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**************************************流式函数************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/*流式比较函数*/
void comparison_r_wrapper(
    hls::stream<r_norm_t>& r_norm_in1,
    hls::stream<r_norm_t>& r_norm_in2,
	hls::stream<r_norm_t>& r_norm_in3,
    hls::stream<r_norm_t>& r_norm_in4,
	// hls::stream<r_norm_t>& r_norm_in5,
    // hls::stream<r_norm_t>& r_norm_in6,
	// hls::stream<r_norm_t>& r_norm_in7,
    // hls::stream<r_norm_t>& r_norm_in8,
    hls::stream<Myreal>& x_real_in1,
    hls::stream<Myimage>& x_imag_in1,
    hls::stream<Myreal>& x_real_in2,
    hls::stream<Myimage>& x_imag_in2,
	hls::stream<Myreal>& x_real_in3,
    hls::stream<Myimage>& x_imag_in3,
    hls::stream<Myreal>& x_real_in4,
    hls::stream<Myimage>& x_imag_in4,
	// hls::stream<Myreal>& x_real_in5,
    // hls::stream<Myimage>& x_imag_in5,
    // hls::stream<Myreal>& x_real_in6,
    // hls::stream<Myimage>& x_imag_in6,
	// hls::stream<Myreal>& x_real_in7,
    // hls::stream<Myimage>& x_imag_in7,
    // hls::stream<Myreal>& x_real_in8,
    // hls::stream<Myimage>& x_imag_in8,
    MyComplex* x_final
){
    // 直接从流中读取数据
    r_norm_t r1 = r_norm_in1.read();
    r_norm_t r2 = r_norm_in2.read();
	r_norm_t r3 = r_norm_in3.read();
    r_norm_t r4 = r_norm_in4.read();
	// r_norm_t r5 = r_norm_in5.read();
    // r_norm_t r6 = r_norm_in6.read();
	// r_norm_t r7 = r_norm_in7.read();
    // r_norm_t r8 = r_norm_in8.read();
    
    MyComplex x1[Ntr_1], x2[Ntr_1], x3[Ntr_1], x4[Ntr_1];//, x5[Ntr_1], x6[Ntr_1], x7[Ntr_1], x8[Ntr_1];
    #pragma HLS ARRAY_PARTITION variable=x1 complete dim=1
    #pragma HLS ARRAY_PARTITION variable=x2 complete dim=1
	#pragma HLS ARRAY_PARTITION variable=x3 complete dim=1
    #pragma HLS ARRAY_PARTITION variable=x4 complete dim=1
	// #pragma HLS ARRAY_PARTITION variable=x5 complete dim=1
    // #pragma HLS ARRAY_PARTITION variable=x6 complete dim=1
	// #pragma HLS ARRAY_PARTITION variable=x7 complete dim=1
    // #pragma HLS ARRAY_PARTITION variable=x8 complete dim=1
    
    for(int i = 0; i < Ntr_1; i++) {
        #pragma HLS unroll
        x1[i].real = x_real_in1.read();
        x1[i].imag = x_imag_in1.read();
        x2[i].real = x_real_in2.read();
        x2[i].imag = x_imag_in2.read();
		x3[i].real = x_real_in3.read();
        x3[i].imag = x_imag_in3.read();
        x4[i].real = x_real_in4.read();
        x4[i].imag = x_imag_in4.read();
		//x5[i].real = x_real_in5.read();
        //x5[i].imag = x_imag_in5.read();
        //x6[i].real = x_real_in6.read();
        //x6[i].imag = x_imag_in6.read();
		//x7[i].real = x_real_in7.read();
        //x7[i].imag = x_imag_in7.read();
        //x8[i].real = x_real_in8.read();
        //x8[i].imag = x_imag_in8.read();
    }
    
    comparison_r(r1, r2, r3, r4, //r5, r6, r7, r8,
		x1, x2, x3, x4, //x5, x6, x7, x8, 
		x_final);
}
// /*shared data calculation*/
// void shared_data_cal(
// 	// in ＆ out接口
//     hls::stream<H_real_t>& H_real_stream, hls::stream<H_imag_t>& H_imag_stream,
//     hls::stream<y_real_t>& y_real_stream, hls::stream<y_imag_t>& y_imag_stream,
//     hls::stream<v_real_t>& v_tb_real_stream, hls::stream<v_imag_t>& v_tb_imag_stream,
//     float sigma2,
// 	// 输出接口
//     hls::stream<like_float>& dqam_fifo_1, 
// 	hls::stream<Myreal>& constellation_norm_real_1,
// 	hls::stream<Myimage>& constellation_norm_imag_1,
// 	hls::stream<grad_preconditioner_real_t>& grad_preconditioner_real_1,
// 	hls::stream<grad_preconditioner_imag_t>& grad_preconditioner_imag_1,
// 	hls::stream<pmat_real_t>& pmat_real_1,
// 	hls::stream<pmat_imag_t>& pmat_imag_1,
// 	hls::stream<like_float>& alpha_fifo_1,
// 
// 	hls::stream<like_float>& dqam_fifo_2, 
// 	hls::stream<Myreal>& constellation_norm_real_2,
// 	hls::stream<Myimage>& constellation_norm_imag_2,
// 	hls::stream<grad_preconditioner_real_t>& grad_preconditioner_real_2,
// 	hls::stream<grad_preconditioner_imag_t>& grad_preconditioner_imag_2,
// 	hls::stream<pmat_real_t>& pmat_real_2,
// 	hls::stream<pmat_imag_t>& pmat_imag_2,
// 	hls::stream<like_float>& alpha_fifo_2,
// 
// 	hls::stream<like_float>& dqam_fifo_3, 
// 	hls::stream<Myreal>& constellation_norm_real_3,
// 	hls::stream<Myimage>& constellation_norm_imag_3,
// 	hls::stream<grad_preconditioner_real_t>& grad_preconditioner_real_3,
// 	hls::stream<grad_preconditioner_imag_t>& grad_preconditioner_imag_3,
// 	hls::stream<pmat_real_t>& pmat_real_3,
// 	hls::stream<pmat_imag_t>& pmat_imag_3,
// 	hls::stream<like_float>& alpha_fifo_3,
// 
// 	hls::stream<like_float>& dqam_fifo_4, 
// 	hls::stream<Myreal>& constellation_norm_real_4,
// 	hls::stream<Myimage>& constellation_norm_imag_4,
// 	hls::stream<grad_preconditioner_real_t>& grad_preconditioner_real_4,
// 	hls::stream<grad_preconditioner_imag_t>& grad_preconditioner_imag_4,
// 	hls::stream<pmat_real_t>& pmat_real_4,
// 	hls::stream<pmat_imag_t>& pmat_imag_4,
// 	hls::stream<like_float>& alpha_fifo_4,
// 
// 	hls::stream<H_real_t>& H_real_out1, hls::stream<H_imag_t>& H_imag_out1,
//     hls::stream<y_real_t>& y_real_out1, hls::stream<y_imag_t>& y_imag_out1,
//     hls::stream<v_real_t>& v_tb_real_out1, hls::stream<v_imag_t>& v_tb_imag_out1,
//     hls::stream<H_real_t>& H_real_out2, hls::stream<H_imag_t>& H_imag_out2,
//     hls::stream<y_real_t>& y_real_out2, hls::stream<y_imag_t>& y_imag_out2,
//     hls::stream<v_real_t>& v_tb_real_out2, hls::stream<v_imag_t>& v_tb_imag_out2,
// 	
// 	hls::stream<H_real_t>& H_real_out3, hls::stream<H_imag_t>& H_imag_out3,
//     hls::stream<y_real_t>& y_real_out3, hls::stream<y_imag_t>& y_imag_out3,
//     hls::stream<v_real_t>& v_tb_real_out3, hls::stream<v_imag_t>& v_tb_imag_out3,
//     hls::stream<H_real_t>& H_real_out4, hls::stream<H_imag_t>& H_imag_out4,
//     hls::stream<y_real_t>& y_real_out4, hls::stream<y_imag_t>& y_imag_out4,
//     hls::stream<v_real_t>& v_tb_real_out4, hls::stream<v_imag_t>& v_tb_imag_out4
// ){
// 	// 本地变量
// 	H_real_t H_real[Ntr_2];
// 	H_imag_t H_imag[Ntr_2];
// 	y_real_t y_real[Ntr_1];
// 	y_imag_t y_imag[Ntr_1];
// 	v_real_t v_tb_real[Ntr_1 * iter_1];
// 	v_imag_t v_tb_imag[Ntr_1 * iter_1];
// 	MyComplex_y y_local[Ntr_1];
// 	MyComplex_H H_local[Ntr_2];
// 	MyComplex_v v_tb_local[Ntr_1 * iter_1];
// 	like_float alpha;
// 	like_float dqam;
// 	MyComplex_grad_preconditioner grad_preconditioner[Ntr_2];
// 	MyComplex constellation_norm[mu_double];/*depend on 2^mu*/
// 	MyComplex_pmat pmat[Ntr_2];
// 	MyComplex_HH HH_H[Ntr_2];
// 	MyComplex_sigma2eye sigma2eye[Ntr_2];
// 	float sigma2_local = sigma2;
// 	/*fifo data read*/
// 	for(int i=0; i<Ntr_2; ++i){
// 		H_real[i] = H_real_stream.read();
// 		H_imag[i] = H_imag_stream.read();
// 	}
// 	for(int i=0; i<Ntr_1; ++i){
// 		y_real[i] = y_real_stream.read();
// 		y_imag[i] = y_imag_stream.read();
// 	}
// 	for(int i=0; i<Ntr_1*iter_1; ++i){
// 		v_tb_real[i] = v_tb_real_stream.read();
// 		v_tb_imag[i] = v_tb_imag_stream.read();
// 	}
// 	/*********************************核心阶段，数据准备************************************/
// 	data_local<MyComplex_H, MyComplex_y, MyComplex_v, H_real_t, H_imag_t, y_real_t, y_imag_t, v_real_t, v_imag_t>(H_local, y_local, v_tb_local, H_real, H_imag, y_real, y_imag, v_tb_real, v_tb_imag);
// 	/*定义发送符号之间最小距离的一半，是星座点经过归一化处理后的结果*/
// 	get_dqam_hw(dqam);
//     /*初始化constellation_norm*/
// 	constellation_norm_initial(constellation_norm, dqam);
// 	/*二阶梯度下降，计算grad_preconditioner(梯度更新的预条件矩阵)*/
// 	grad_preconditioner_updater_hw(H_local, HH_H, sigma2eye, grad_preconditioner, sigma2_local, dqam);
// 	/*alpha*/
// 	get_alpha(alpha);
//     /*For learning rate line search */
//     learning_rate_line_search_hw<MyComplex_H, MyComplex_grad_preconditioner, MyComplex_pmat>(lr_approx_1, H_local, grad_preconditioner, Ntr_1, Ntr_1, pmat);
// 	
// 	/*Output phase fifo data write*/
// 	for(int i=0; i<Ntr_2; ++i){
// 		H_real_stream.write(H_real[i]);
// 		H_imag_stream.write(H_imag[i]);
// 	}
// 	for(int i=0; i<Ntr_1; ++i){
// 		y_real_stream.write(y_real[i]);
// 		y_imag_stream.write(y_imag[i]);
// 	}
// 	for(int i=0; i<Ntr_1*iter_1; ++i){
// 		v_tb_real_stream.write(v_tb_real[i]);
// 		v_tb_imag_stream.write(v_tb_imag[i]);
// 	}
// 	data_distribution(
// 		H_real_stream, H_imag_stream, y_real_stream, y_imag_stream, 
// 		v_tb_real_stream, v_tb_imag_stream,
// 		v_tb_real_2, v_tb_imag_2,
// 		v_tb_real_3, v_tb_imag_3,
// 		v_tb_real_4, v_tb_imag_4,
// 		// v_tb_real_5, v_tb_imag_5,
// 		// v_tb_real_6, v_tb_imag_6,
// 		// v_tb_real_7, v_tb_imag_7,
// 		// v_tb_real_8, v_tb_imag_8,
// 
// 		H_real_stream_1, H_imag_stream_1,
// 		y_real_stream_1, y_imag_stream_1,
// 		v_tb_real_stream_1, v_tb_imag_stream_1,
// 		H_real_stream_2, H_imag_stream_2,
// 		y_real_stream_2, y_imag_stream_2,
// 		v_tb_real_stream_2, v_tb_imag_stream_2,
// 
// 		H_real_stream_3, H_imag_stream_3,
// 		y_real_stream_3, y_imag_stream_3,
// 		v_tb_real_stream_3, v_tb_imag_stream_3,
// 		H_real_stream_4, H_imag_stream_4,
// 		y_real_stream_4, y_imag_stream_4,
// 		v_tb_real_stream_4, v_tb_imag_stream_4
// 
// 		// H_real_stream_5, H_imag_stream_5,
// 		// y_real_stream_5, y_imag_stream_5,
// 		// v_tb_real_stream_5, v_tb_imag_stream_5,
// 		// H_real_stream_6, H_imag_stream_6,
// 		// y_real_stream_6, y_imag_stream_6,
// 		// v_tb_real_stream_6, v_tb_imag_stream_6,
// 
// 		// H_real_stream_7, H_imag_stream_7,
// 		// y_real_stream_7, y_imag_stream_7,
// 		// v_tb_real_stream_7, v_tb_imag_stream_7,
// 		// H_real_stream_8, H_imag_stream_8,
// 		// y_real_stream_8, y_imag_stream_8,
// 		// v_tb_real_stream_8, v_tb_imag_stream_8
// 	);
// 	for(int i=0; i<mu_double; ++i){
// 		constellation_norm_real.write(constellation_norm[i].real);
// 		constellation_norm_imag.write(constellation_norm[i].imag);
// 	}
// 	for(int i=0; i<Ntr_2; ++i){
// 		grad_preconditioner_real.write(grad_preconditioner[i].real);
// 		grad_preconditioner_imag.write(grad_preconditioner[i].imag);
// 	}
// 	for(int i=0; i<Ntr_2; ++i){
// 		pmat_real.write(pmat[i].real);
// 		pmat_imag.write(pmat[i].imag);
// 	}
// 	dqam_fifo.write(dqam);
// 	alpha_fifo.write(alpha);
// }
// /*完全独立的单采样器函数*/
// void sampler_task_1(
//     // 输入接口
//     hls::stream<H_real_t>& H_real_stream, hls::stream<H_imag_t>& H_imag_stream,
//     hls::stream<y_real_t>& y_real_stream, hls::stream<y_imag_t>& y_imag_stream,
//     hls::stream<v_real_t>& v_tb_real_stream, hls::stream<v_imag_t>& v_tb_imag_stream,
// 	hls::stream<like_float>& dqam_fifo, 
// 	hls::stream<Myreal>& constellation_norm_real, hls::stream<Myimage>& constellation_norm_imag,
// 	hls::stream<grad_preconditioner_real_t>& grad_preconditioner_real, hls::stream<grad_preconditioner_imag_t>& grad_preconditioner_imag,
// 	hls::stream<pmat_real_t>& pmat_real, hls::stream<pmat_imag_t>& pmat_imag,
// 	hls::stream<like_float>& alpha_fifo,
//     float sigma2,
//     int sampler_id,
// 	unsigned seed_in,
//     // 输出接口
//     hls::stream<Myreal>& x_survivor_real, hls::stream<Myimage>& x_survivor_imag,
// 	hls::stream<r_norm_t>& r_norm_survivor_out
// ){
// 	//本地变量
// 	H_real_t H_real[Ntr_2];
// 	H_imag_t H_imag[Ntr_2];
// 	y_real_t y_real[Ntr_1];
// 	y_imag_t y_imag[Ntr_1];
// 	v_real_t v_tb_real[Ntr_1 * iter_1];
// 	v_imag_t v_tb_imag[Ntr_1 * iter_1];
// 	MyComplex x_hat[Ntr_1];
// 	MyComplex_y y_local[Ntr_1];
// 	MyComplex_H H_local[Ntr_2];
// 	MyComplex_v v_tb_local[Ntr_1 * iter_1];
// 	like_float alpha;
// 	like_float dqam;
// 	step_size_t step_size;
// 	r_norm_t r_norm;/*the norm of residual vector*/
// 	r_norm_t r_norm_survivor;
// 	lr_t lr;/*learning rate*/
// 	MyComplex_grad_preconditioner grad_preconditioner[Ntr_2];
// 	MyComplex constellation_norm[mu_double];/*depend on 2^mu*/
// 	MyComplex_pmat pmat[Ntr_2];
// 	MyComplex x_survivor[Ntr_1];
// 	MyComplex_r r[Ntr_1];
// 	MyComplex_pr_prev pr_prev[Ntr_1];
// 	MyComplex_HH HH_H[Ntr_2];
// 	MyComplex_sigma2eye sigma2eye[Ntr_2];
// 	int offset = 0;
// 	float sigma2_local = sigma2;
// 	int lr_approx = lr_approx_1;
// 	int mmse_init = mmse_init_1;
// 	unsigned int seed = seed_in;
// 	for(int i=0; i<Ntr_2; ++i){
// 		H_real[i] = H_real_stream.read();
// 		H_imag[i] = H_imag_stream.read();
// 	}
// 	for(int i=0; i<Ntr_1; ++i){
// 		y_real[i] = y_real_stream.read();
// 		y_imag[i] = y_imag_stream.read();
// 	}
// 	for(int i=0; i<Ntr_1*iter_1; ++i){
// 		v_tb_real[i] = v_tb_real_stream.read();
// 		v_tb_imag[i] = v_tb_imag_stream.read();
// 	}
// 	for(int i=0; i<mu_double; ++i){
// 		constellation_norm[i].real = constellation_norm_real.read();
// 		constellation_norm[i].imag = constellation_norm_imag.read();
// 	}
// 	for(int i=0; i<Ntr_2; ++i){
// 		grad_preconditioner[i].real = grad_preconditioner_real.read();
// 		grad_preconditioner[i].imag = grad_preconditioner_imag.read();
// 	}
// 	for(int i=0; i<Ntr_2; ++i){
// 		pmat[i].real = pmat_real.read();
// 		pmat[i].imag = pmat_imag.read();
// 	}
// 	dqam = dqam_fifo.read();
// 	alpha = alpha_fifo.read();
// 
// 	/*********************************数据准备************************************/
// 	data_local<MyComplex_H, MyComplex_y, MyComplex_v, H_real_t, H_imag_t, y_real_t, y_imag_t, v_real_t, v_imag_t>(H_local, y_local, v_tb_local, H_real, H_imag, y_real, y_imag, v_tb_real, v_tb_imag);
// 	/*定义发送符号之间最小距离的一半，是星座点经过归一化处理后的结果*/
// 	// get_dqam_hw(dqam);
//     /*初始化constellation_norm*/
// 	// constellation_norm_initial(constellation_norm, dqam);
// 	/*二阶梯度下降，计算grad_preconditioner(梯度更新的预条件矩阵)*/
// 	// grad_preconditioner_updater_hw(H_local, HH_H, sigma2eye, grad_preconditioner, sigma2_local, dqam);
// 	/*alpha*/
// 	// get_alpha(alpha);
//     /*For learning rate line search */
//     // learning_rate_line_search_hw<MyComplex_H, MyComplex_grad_preconditioner, MyComplex_pmat>(lr_approx, H_local, grad_preconditioner, Ntr_1, Ntr_1, pmat);
//     /*x的初始化*/
//     x_initialize_hw(mmse_init, sigma2eye, Ntr_1, Ntr_1, sigma2_local, HH_H, H_local, y_local, sampler_id, dqam, x_hat, constellation_norm, seed);
// 	/*计算剩余向量r=y-Hx*/
//     r_hw(H_local, x_hat, r, y_local);
//     /*计算剩余向量的范数（就是模值）*/
// 	r_cal_hw(r, x_hat, x_survivor, r_norm, r_norm_survivor);
//     /*确定最优学习率*/
// 	lr_hw(lr_approx, pmat, r, pr_prev, lr, sampler_id);
//     /*步长初始化*/
// 	step_size_hw(step_size, alpha, dqam, r_norm);
// 	/*********************************核心计算************************************/
// 	samplers_process(
// 		/*静态量*/
// 		H_local, y_local, v_tb_local, grad_preconditioner,
// 		pmat, constellation_norm, dqam, alpha, sigma2_local, lr_approx_1, sampler_id,
// 		/*动态*/
// 		x_hat, r, r_norm, pr_prev, lr,
// 		step_size, offset, x_survivor, r_norm_survivor, seed
// 	);
// 	/*********************************结果输出************************************/
// 	r_norm_survivor_out.write(r_norm_survivor);
// 	for(int i=0; i<Ntr_1; ++i){
// 		#pragma HLS PIPELINE II=1
// 		x_survivor_real.write(x_survivor[i].real);
// 		x_survivor_imag.write(x_survivor[i].imag);
// 	}
// }
/*完全独立的单采样器函数*/
void sampler_task(
    // 输入接口
    hls::stream<H_real_t>& H_real_stream, hls::stream<H_imag_t>& H_imag_stream,
    hls::stream<y_real_t>& y_real_stream, hls::stream<y_imag_t>& y_imag_stream,
    hls::stream<v_real_t>& v_tb_real_stream, hls::stream<v_imag_t>& v_tb_imag_stream,
    float sigma2,
    int sampler_id,
	unsigned seed_in,
    // 输出接口
    hls::stream<Myreal>& x_survivor_real, hls::stream<Myimage>& x_survivor_imag,
	hls::stream<r_norm_t>& r_norm_survivor_out
){
	//本地变量
	H_real_t H_real[Ntr_2];
	H_imag_t H_imag[Ntr_2];
	y_real_t y_real[Ntr_1];
	y_imag_t y_imag[Ntr_1];
	v_real_t v_tb_real[Ntr_1 * iter_1];
	v_imag_t v_tb_imag[Ntr_1 * iter_1];
	MyComplex x_hat[Ntr_1];
	MyComplex_y y_local[Ntr_1];
	MyComplex_H H_local[Ntr_2];
	MyComplex_v v_tb_local[Ntr_1 * iter_1];
	like_float alpha;
	like_float dqam;
	step_size_t step_size;
	r_norm_t r_norm;/*the norm of residual vector*/
	r_norm_t r_norm_survivor;
	lr_t lr;/*learning rate*/
	MyComplex_grad_preconditioner grad_preconditioner[Ntr_2];
	MyComplex constellation_norm[mu_double];/*depend on 2^mu*/
	MyComplex_pmat pmat[Ntr_2];
	MyComplex x_survivor[Ntr_1];
	MyComplex_r r[Ntr_1];
	MyComplex_pr_prev pr_prev[Ntr_1];
	MyComplex_HH HH_H[Ntr_2];
	MyComplex_sigma2eye sigma2eye[Ntr_2];
	int offset = 0;
	float sigma2_local = sigma2;
	int lr_approx = lr_approx_1;
	int mmse_init = mmse_init_1;
	unsigned int seed = seed_in;
	for(int i=0; i<Ntr_2; ++i){
		H_real[i] = H_real_stream.read();
		H_imag[i] = H_imag_stream.read();
	}
	for(int i=0; i<Ntr_1; ++i){
		y_real[i] = y_real_stream.read();
		y_imag[i] = y_imag_stream.read();
	}
	for(int i=0; i<Ntr_1*iter_1; ++i){
		v_tb_real[i] = v_tb_real_stream.read();
		v_tb_imag[i] = v_tb_imag_stream.read();
	}

	/*********************************数据准备************************************/
	data_local<MyComplex_H, MyComplex_y, MyComplex_v, H_real_t, H_imag_t, y_real_t, y_imag_t, v_real_t, v_imag_t>(H_local, y_local, v_tb_local, H_real, H_imag, y_real, y_imag, v_tb_real, v_tb_imag);
	/*定义发送符号之间最小距离的一半，是星座点经过归一化处理后的结果*/
	get_dqam_hw(dqam);
    /*初始化constellation_norm*/
	constellation_norm_initial(constellation_norm, dqam);
	/*二阶梯度下降，计算grad_preconditioner(梯度更新的预条件矩阵)*/
	grad_preconditioner_updater_hw(H_local, HH_H, sigma2eye, grad_preconditioner, sigma2_local, dqam);
	/*alpha*/
	get_alpha(alpha);
    /*For learning rate line search */
    learning_rate_line_search_hw<MyComplex_H, MyComplex_grad_preconditioner, MyComplex_pmat>(lr_approx, H_local, grad_preconditioner, Ntr_1, Ntr_1, pmat);
    /*x的初始化*/
    x_initialize_hw(mmse_init, sigma2eye, Ntr_1, Ntr_1, sigma2_local, HH_H, H_local, y_local, sampler_id, dqam, x_hat, constellation_norm, seed);
	/*计算剩余向量r=y-Hx*/
    r_hw(H_local, x_hat, r, y_local);
    /*计算剩余向量的范数（就是模值）*/
	r_cal_hw(r, x_hat, x_survivor, r_norm, r_norm_survivor);
    /*确定最优学习率*/
	lr_hw(lr_approx, pmat, r, pr_prev, lr, sampler_id);
    /*步长初始化*/
	step_size_hw(step_size, alpha, dqam, r_norm);
	/*********************************核心计算************************************/
	samplers_process(
		/*静态量*/
		H_local, y_local, v_tb_local, grad_preconditioner,
		pmat, constellation_norm, dqam, alpha, sigma2_local, lr_approx_1, sampler_id,
		/*动态*/
		x_hat, r, r_norm, pr_prev, lr,
		step_size, offset, x_survivor, r_norm_survivor, seed
	);
	/*********************************结果输出************************************/
	r_norm_survivor_out.write(r_norm_survivor);
	for(int i=0; i<Ntr_1; ++i){
		#pragma HLS PIPELINE II=1
		x_survivor_real.write(x_survivor[i].real);
		x_survivor_imag.write(x_survivor[i].imag);
	}
}


/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/***************************************顶层函数***********************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/

void MHGD_detect_accel_hw(
    Myreal* x_hat_real, Myimage* x_hat_imag, 
    H_real_t* H_real, H_imag_t* H_imag, 
    y_real_t* y_real, y_imag_t* y_imag, 
    v_real_t* v_tb_real, v_imag_t* v_tb_imag,
	v_real_t* v_tb_real_2, v_imag_t* v_tb_imag_2,
	v_real_t* v_tb_real_3, v_imag_t* v_tb_imag_3,
	v_real_t* v_tb_real_4, v_imag_t* v_tb_imag_4,
	//v_real_t* v_tb_real_5, v_imag_t* v_tb_imag_5,
	//v_real_t* v_tb_real_6, v_imag_t* v_tb_imag_6,
	//v_real_t* v_tb_real_7, v_imag_t* v_tb_imag_7,
	//v_real_t* v_tb_real_8, v_imag_t* v_tb_imag_8,
	float sigma2, 
	unsigned int seed_1, unsigned int seed_2, unsigned int seed_3, unsigned int seed_4 //unsigned int seed_5, unsigned int seed_6, unsigned int seed_7, unsigned int seed_8
){
	/****************************AXI-Master 接口配置*******************************/
    #pragma HLS INTERFACE mode=m_axi port=x_hat_real depth=Ntr_1 offset=slave
    #pragma HLS INTERFACE mode=m_axi port=x_hat_imag depth=Ntr_1 offset=slave
    #pragma HLS INTERFACE mode=m_axi port=H_real depth=Ntr_2 offset=slave
    #pragma HLS INTERFACE mode=m_axi port=H_imag depth=Ntr_2 offset=slave
    #pragma HLS INTERFACE mode=m_axi port=y_real depth=Ntr_1 offset=slave
    #pragma HLS INTERFACE mode=m_axi port=y_imag depth=Ntr_1 offset=slave
    #pragma HLS INTERFACE mode=m_axi port=v_tb_real depth=num_ran offset=slave
    #pragma HLS INTERFACE mode=m_axi port=v_tb_imag depth=num_ran offset=slave
	#pragma HLS INTERFACE mode=m_axi port=v_tb_real_2 depth=num_ran offset=slave
    #pragma HLS INTERFACE mode=m_axi port=v_tb_imag_2 depth=num_ran offset=slave
	#pragma HLS INTERFACE mode=m_axi port=v_tb_real_3 depth=num_ran offset=slave
    #pragma HLS INTERFACE mode=m_axi port=v_tb_imag_3 depth=num_ran offset=slave
	#pragma HLS INTERFACE mode=m_axi port=v_tb_real_4 depth=num_ran offset=slave
    #pragma HLS INTERFACE mode=m_axi port=v_tb_imag_4 depth=num_ran offset=slave
	// #pragma HLS INTERFACE mode=m_axi port=v_tb_real_5 depth=num_ran offset=slave
    // #pragma HLS INTERFACE mode=m_axi port=v_tb_imag_5 depth=num_ran offset=slave
	// #pragma HLS INTERFACE mode=m_axi port=v_tb_real_6 depth=num_ran offset=slave
    // #pragma HLS INTERFACE mode=m_axi port=v_tb_imag_6 depth=num_ran offset=slave
	// #pragma HLS INTERFACE mode=m_axi port=v_tb_real_7 depth=num_ran offset=slave
    // #pragma HLS INTERFACE mode=m_axi port=v_tb_imag_7 depth=num_ran offset=slave
	// #pragma HLS INTERFACE mode=m_axi port=v_tb_real_8 depth=num_ran offset=slave
    // #pragma HLS INTERFACE mode=m_axi port=v_tb_imag_8 depth=num_ran offset=slave

	//输入数据转换为流数据
	hls::stream<H_real_t> H_real_stream[samplers];
    hls::stream<H_imag_t> H_imag_stream[samplers];
    hls::stream<y_real_t> y_real_stream[samplers];
    hls::stream<y_imag_t> y_imag_stream[samplers];
    hls::stream<v_real_t> v_tb_real_stream[samplers];
    hls::stream<v_imag_t> v_tb_imag_stream[samplers];
	#pragma HLS STREAM variable=H_real_stream depth=256
    #pragma HLS STREAM variable=H_imag_stream depth=256
	#pragma HLS STREAM variable=y_real_stream depth=256
    #pragma HLS STREAM variable=y_imag_stream depth=256
	#pragma HLS STREAM variable=v_tb_real_stream depth=256
    #pragma HLS STREAM variable=v_tb_imag_stream depth=256
	//采样器结果
	hls::stream<Myreal> x_survivor_real_1;
    hls::stream<Myimage> x_survivor_imag_1;
    hls::stream<Myreal> x_survivor_real_2;
    hls::stream<Myimage> x_survivor_imag_2;
	hls::stream<Myreal> x_survivor_real_3;
    hls::stream<Myimage> x_survivor_imag_3;
	hls::stream<Myreal> x_survivor_real_4;
    hls::stream<Myimage> x_survivor_imag_4;
	// hls::stream<Myreal> x_survivor_real_5;
    // hls::stream<Myimage> x_survivor_imag_5;
	// hls::stream<Myreal> x_survivor_real_6;
    // hls::stream<Myimage> x_survivor_imag_6;
	// hls::stream<Myreal> x_survivor_real_7;
    // hls::stream<Myimage> x_survivor_imag_7;
	// hls::stream<Myreal> x_survivor_real_8;
    // hls::stream<Myimage> x_survivor_imag_8;
    hls::stream<r_norm_t> r_norm_survivor_out_1_stream;
    hls::stream<r_norm_t> r_norm_survivor_out_2_stream;
	hls::stream<r_norm_t> r_norm_survivor_out_3_stream;
    hls::stream<r_norm_t> r_norm_survivor_out_4_stream;
	// hls::stream<r_norm_t> r_norm_survivor_out_5_stream;
    // hls::stream<r_norm_t> r_norm_survivor_out_6_stream;
	// hls::stream<r_norm_t> r_norm_survivor_out_7_stream;
	// hls::stream<r_norm_t> r_norm_survivor_out_8_stream;

	#pragma HLS STREAM variable=x_survivor_real_1 depth=Ntr_1
    #pragma HLS STREAM variable=x_survivor_imag_1 depth=Ntr_1
	#pragma HLS STREAM variable=x_survivor_real_2 depth=Ntr_1
    #pragma HLS STREAM variable=x_survivor_imag_2 depth=Ntr_1
	#pragma HLS STREAM variable=x_survivor_real_3 depth=Ntr_1
    #pragma HLS STREAM variable=x_survivor_imag_3 depth=Ntr_1
	#pragma HLS STREAM variable=x_survivor_real_4 depth=Ntr_1
    #pragma HLS STREAM variable=x_survivor_imag_4 depth=Ntr_1
	// #pragma HLS STREAM variable=x_survivor_real_5 depth=Ntr_1
    // #pragma HLS STREAM variable=x_survivor_imag_5 depth=Ntr_1
	// #pragma HLS STREAM variable=x_survivor_real_6 depth=Ntr_1
    // #pragma HLS STREAM variable=x_survivor_imag_6 depth=Ntr_1
	// #pragma HLS STREAM variable=x_survivor_real_7 depth=Ntr_1
    // #pragma HLS STREAM variable=x_survivor_imag_7 depth=Ntr_1
	// #pragma HLS STREAM variable=x_survivor_real_8 depth=Ntr_1
    // #pragma HLS STREAM variable=x_survivor_imag_8 depth=Ntr_1
	#pragma HLS STREAM variable=r_norm_survivor_out_1_stream depth=1
    #pragma HLS STREAM variable=r_norm_survivor_out_2_stream depth=1
	#pragma HLS STREAM variable=r_norm_survivor_out_3_stream depth=1
	#pragma HLS STREAM variable=r_norm_survivor_out_4_stream depth=1
	// #pragma HLS STREAM variable=r_norm_survivor_out_5_stream depth=1
	// #pragma HLS STREAM variable=r_norm_survivor_out_6_stream depth=1
	// #pragma HLS STREAM variable=r_norm_survivor_out_7_stream depth=1
	// #pragma HLS STREAM variable=r_norm_survivor_out_8_stream depth=1

	MyComplex x_survivor_final[Ntr_1];
	#pragma HLS ARRAY_PARTITION variable=x_survivor_final complete dim=1
	int sampler_id_1 = 1;
	int sampler_id_2 = 2;
	int sampler_id_3 = 3;
	int sampler_id_4 = 4;
	// int sampler_id_5 = 5;
	// int sampler_id_6 = 6;
	// int sampler_id_7 = 7;
	// int sampler_id_8 = 8;
	float sigma2_1 = sigma2;
	float sigma2_2 = sigma2;
	float sigma2_3 = sigma2;
	float sigma2_4 = sigma2;
	// float sigma2_5 = sigma2;
	// float sigma2_6 = sigma2;
	// float sigma2_7 = sigma2;
	// float sigma2_8 = sigma2;
	unsigned int seed1 = seed_1;
	unsigned int seed2 = seed_2;
	unsigned int seed3 = seed_3;
	unsigned int seed4 = seed_4;
	// unsigned int seed5 = seed_5;
	// unsigned int seed6 = seed_6;
	// unsigned int seed7 = seed_7;
	// unsigned int seed8 = seed_8;
	#pragma HLS dataflow
	/**************************** 数据分发 *******************************/
    H_DISTRIBUTE:
    for(int i = 0; i < Ntr_2; ++i) {
        #pragma HLS PIPELINE II=1
        H_real_t h_real = H_real[i];
        H_imag_t h_imag = H_imag[i];
		for (int s = 0; s < samplers; ++s) {
			#pragma HLS UNROLL
			H_real_stream[s].write(h_real);
			H_imag_stream[s].write(h_imag);
		}
    }
    Y_DISTRIBUTE:
    for(int i = 0; i < Ntr_1; ++i) {
        #pragma HLS PIPELINE II=1
        y_real_t y_r = y_real[i];
        y_imag_t y_i = y_imag[i];
		for (int s = 0; s < samplers; ++s) {
			#pragma HLS UNROLL
			y_real_stream[s].write(y_r);
			y_imag_stream[s].write(y_i);
		}
    }
    V_DISTRIBUTE:
    for(int i = 0; i < Ntr_1 * iter_1; ++i) {
        #pragma HLS PIPELINE II=1
		v_real_t v_r[samplers];
		v_imag_t v_i[samplers];
		#pragma HLS ARRAY_PARTITION variable=v_r complete dim=1
		#pragma HLS ARRAY_PARTITION variable=v_i complete dim=1
        v_r[0] = v_tb_real[i];
        v_i[0] = v_tb_imag[i];
		v_r[1] = v_tb_real_2[i];
        v_i[1] = v_tb_imag_2[i];
		v_r[2] = v_tb_real_3[i];
        v_i[2] = v_tb_imag_3[i];
		v_r[3] = v_tb_real_4[i];
        v_i[3] = v_tb_imag_4[i];
		for (int s = 0; s < samplers; ++s) {
			#pragma HLS UNROLL
			v_tb_real_stream[s].write(v_r[s]);
			v_tb_imag_stream[s].write(v_i[s]);
		}
    }
	/****************************采样器并行采样*******************************/
	// #pragma HLS allocation instances=sampler_task limit=2 function
	sampler_task(
		// 输入接口
		H_real_stream[0], H_imag_stream[0], 
		y_real_stream[0], y_imag_stream[0], 
		v_tb_real_stream[0], v_tb_imag_stream[0], 
		sigma2_1, sampler_id_1, seed1,
		// 输出接口
		x_survivor_real_1, x_survivor_imag_1, 
		r_norm_survivor_out_1_stream
	);
	sampler_task(
		// 输入接口
		H_real_stream[1], H_imag_stream[1], 
		y_real_stream[1], y_imag_stream[1], 
		v_tb_real_stream[1], v_tb_imag_stream[1], 
		sigma2_2, sampler_id_2, seed2,
		// 输出接口
		x_survivor_real_2, x_survivor_imag_2, 
		r_norm_survivor_out_2_stream
	);
	sampler_task(
		// 输入接口
		H_real_stream[2], H_imag_stream[2], 
		y_real_stream[2], y_imag_stream[2], 
		v_tb_real_stream[2], v_tb_imag_stream[2], 
		sigma2_3, sampler_id_3, seed3,
		// 输出接口
		x_survivor_real_3, x_survivor_imag_3, 
		r_norm_survivor_out_3_stream
	);
	sampler_task(
		// 输入接口
		H_real_stream[3], H_imag_stream[3], 
		y_real_stream[3], y_imag_stream[3], 
		v_tb_real_stream[3], v_tb_imag_stream[3], 
		sigma2_4, sampler_id_4, seed4,
		// 输出接口
		x_survivor_real_4, x_survivor_imag_4, 
		r_norm_survivor_out_4_stream
	);
	// sampler_task(
	// 	// 输入接口
	// 	H_real_stream_5, H_imag_stream_5, 
	// 	y_real_stream_5, y_imag_stream_5, 
	// 	v_tb_real_stream_5, v_tb_imag_stream_5, 
	// 	sigma2_5, sampler_id_5, seed5,
	// 	// 输出接口
	// 	x_survivor_real_5, x_survivor_imag_5, 
	// 	r_norm_survivor_out_5_stream
	// );
	// sampler_task(
	// 	// 输入接口
	// 	H_real_stream_6, H_imag_stream_6, 
	// 	y_real_stream_6, y_imag_stream_6, 
	// 	v_tb_real_stream_6, v_tb_imag_stream_6, 
	// 	sigma2_6, sampler_id_6, seed6,
	// 	// 输出接口
	// 	x_survivor_real_6, x_survivor_imag_6, 
	// 	r_norm_survivor_out_6_stream
	// );
	// sampler_task(
	// 	// 输入接口
	// 	H_real_stream_7, H_imag_stream_7, 
	// 	y_real_stream_7, y_imag_stream_7, 
	// 	v_tb_real_stream_7, v_tb_imag_stream_7, 
	// 	sigma2_7, sampler_id_7, seed7,
	// 	// 输出接口
	// 	x_survivor_real_7, x_survivor_imag_7, 
	// 	r_norm_survivor_out_7_stream
	// );
	// sampler_task(
	// 	// 输入接口
	// 	H_real_stream_8, H_imag_stream_8, 
	// 	y_real_stream_8, y_imag_stream_8, 
	// 	v_tb_real_stream_8, v_tb_imag_stream_8, 
	// 	sigma2_8, sampler_id_8, seed8,
	// 	// 输出接口
	// 	x_survivor_real_8, x_survivor_imag_8, 
	// 	r_norm_survivor_out_8_stream
	// );
	/****************************采样结果比较*******************************/
	comparison_r_wrapper(
		r_norm_survivor_out_1_stream, r_norm_survivor_out_2_stream,
		r_norm_survivor_out_3_stream, r_norm_survivor_out_4_stream,
		// r_norm_survivor_out_5_stream, r_norm_survivor_out_6_stream,
		// r_norm_survivor_out_7_stream, r_norm_survivor_out_8_stream,
		x_survivor_real_1, x_survivor_imag_1,
		x_survivor_real_2, x_survivor_imag_2,
		x_survivor_real_3, x_survivor_imag_3,
		x_survivor_real_4, x_survivor_imag_4,
		// x_survivor_real_5, x_survivor_imag_5,
		// x_survivor_real_6, x_survivor_imag_6,
		// x_survivor_real_7, x_survivor_imag_7,
		// x_survivor_real_8, x_survivor_imag_8,
		x_survivor_final
	);
    /****************************迭代结束x_survivor写入输出口*********************************/
    out_hw<MyComplex, Myreal, Myimage>(x_survivor_final, 1, x_hat_real, x_hat_imag, 1);
}
