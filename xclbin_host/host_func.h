#pragma once
#include "MyComplex_1.h"
#include "hls_math.h"


typedef struct _IO_FILE FILE;

//文件路径模板
const char* H_FILE_TEMPLATE = "/home/ggg_wufuqi/hls/MHGD/MHGD/8_8_4QAM/input_file/H_SNR=%.0f.txt";
const char* Y_FILE_TEMPLATE = "/home/ggg_wufuqi/hls/MHGD/MHGD/8_8_4QAM/input_file/y_SNR=%.0f.txt";
const char* BITS_FILE_TEMPLATE = "/home/ggg_wufuqi/hls/MHGD/MHGD/8_8_4QAM/reference_file/bits_SNR=%.0f.txt";
const char* OUTPUT_FILE_TEMPLATE = "/home/ggg_wufuqi/hls/MHGD/MHGD/output_file/bits_output_SNR=%.0f.txt";
const char* GAUSS_TEMPLATE = "/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai/gaussian_random_values_plus.txt";
const char* GAUSS_TEMPLATE_2 = "/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai/gaussian_random_values_plus_2.txt";
const char* GAUSS_TEMPLATE_3 = "/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai/gaussian_random_values_plus_3.txt";
const char* GAUSS_TEMPLATE_4 = "/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai/gaussian_random_values_plus_4.txt";

/*算法参数*/
static const int Ntr_1 = 8;/*发射&接收天线数*/
static const int Ntr_2 = 64;/*NtorNr^2*/
static const int iter_1 = 8;/*采样器的采样数*/
static const int mu_1 = 2;/*调制阶数*/
static const int mmse_init_1 = 0;/*是否使用MMSE检测的结果作为MCMC采样的初始值*/
static const int lr_approx_1 = 0;
static const int samplers = 4; /*采样器数量*/
static const int max_iter_1 = 100;/*希望仿真的最大轮数*/