#include <iostream>
#include <cstring>
#include <fstream>
#include <cmath>
// XRT includes
#include "xrt/xrt_bo.h"
#include <experimental/xrt_xclbin.h>
#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"
// TB includes
#include "host_func.h"
#include "MyComplex_1.h"
#include <string.h>
#include <stdio.h>
#include <chrono>

MyComplex QPSK_Constellation_hw[4] = {{-1,-1},{-1,1},{1,-1},{1,1}};
MyComplex _16QAM_Constellation_hw[16] = {{-3.0,-3.0},{-3.0,-1.0},{-3.0,3.0},{-3.0,1.0},{-1.0,-3.0},{-1.0,-1.0},{-1.0,3.0},{-1.0,1.0},{3.0,-3.0},{3.0,-1.0},{3.0,3.0},{3.0,1.0},{1.0,-3.0},{1.0,-1.0},{1.0,3.0},{1.0,1.0}};
MyComplex _64QAM_Constellation_hw[64] = {{-7.0, 7.0},  {-5.0, 7.0},  {-3.0, 7.0}, {-1.0, 7.0}, 
                                         {1.0, 7.0},   {3.0, 7.0},   {5.0, 7.0},  {7.0, 7.0},
                                         {-7.0, 5.0},  {-5.0, 5.0},  {-3.0, 5.0}, {-1.0, 5.0},
                                         {1.0, 5.0},   {3.0, 5.0},   {5.0, 5.0},  {7.0, 5.0},
                                         {-7.0, 3.0},  {-5.0, 3.0},  {-3.0, 3.0}, {-1.0, 3.0},
                                         {1.0, 3.0},   {3.0, 3.0},   {5.0, 3.0},  {7.0, 3.0},
                                         {-7.0, 1.0},  {-5.0, 1.0},  {-3.0, 1.0}, {-1.0, 1.0},
                                         {1.0, 1.0},   {3.0, 1.0},   {5.0, 1.0},  {7.0, 1.0},
                                         {-7.0, -1.0}, {-5.0, -1.0}, {-3.0, -1.0},{-1.0, -1.0},
                                         {1.0, -1.0},  {3.0, -1.0},  {5.0, -1.0}, {7.0, -1.0},
                                         {-7.0, -3.0}, {-5.0, -3.0}, {-3.0, -3.0},{-1.0, -3.0},
                                         {1.0, -3.0},  {3.0, -3.0},  {5.0, -3.0}, {7.0, -3.0},
                                         {-7.0, -5.0}, {-5.0, -5.0}, {-3.0, -5.0},{-1.0, -5.0},
                                         {1.0, -5.0},  {3.0, -5.0},  {5.0, -5.0}, {7.0, -5.0},
                                         {-7.0, -7.0}, {-5.0, -7.0}, {-3.0, -7.0},{-1.0, -7.0},
                                         {1.0, -7.0},  {3.0, -7.0},  {5.0, -7.0}, {7.0, -7.0}};
void read_gaussian_data_hw(const char* filename, MyComplex* data, int size, int offset);
void read_gaussian_data_hw_1(const char* filename, MyComplex* array, int n, int offset);
void QAM_Demodulation_hw_de(Myreal* x_hat_real, Myimage* x_hat_imag, int Nt, int mu, int* bits_demod);
void QPSK_Demodulation_hw(MyComplex* x_hat, int Nt, int* bits_demod);
void _16QAM_Demodulation_hw(MyComplex* x_hat, int Nt, int* bits_demod);
void _64QAM_Demodulation_hw(MyComplex* x_hat, int Nt, int* bits_demod);
int argmin_hw(like_float* array, int n);
int unequal_times_hw(int* array1, int* array2, int n);


void read_gaussian_data_hw(const char* filename, MyComplex* data, int size, int offset) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) throw std::runtime_error("Failed to open Gaussian data file");
    file.seekg(offset * sizeof(MyComplex));
    file.read(reinterpret_cast<char*>(data), size * sizeof(MyComplex));
}

void read_gaussian_data_hw_1(const char* filename, MyComplex_v* array, int n, int offset) {
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
			printf("Error reading data!\n");
			fclose(f);
			return;  // 返回已成功读取的数据数量
		}
		// 将读取的实部和虚部数据存入数组
		array[i].real = hls_internal::generic_divide((ap_fixed<64,32>)temp_real, (ap_fixed<64,32>)local_temp);
		array[i].imag = hls_internal::generic_divide((ap_fixed<64,32>)temp_imag, (ap_fixed<64,32>)local_temp);
	}
	fclose(f);
}


void QAM_Demodulation_hw_de(MyComplex* x_hat, int Nt, int mu, int* bits_demod)
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
			temp_1[j].real = x_hat[i].real * hls::sqrt((ap_fixed<64,32>)2.0);
			temp_1[j].imag = x_hat[i].imag * hls::sqrt((ap_fixed<64,32>)2.0);
			temp[j].real = temp_1[j].real - QPSK_Constellation_hw[j].real;
			temp[j].imag = temp_1[j].imag - QPSK_Constellation_hw[j].imag;
		}
		for (j = 0; j < 4; j++)
		{
			temp_2 = (temp[j].real * temp[j].real + temp[j].imag * temp[j].imag);
			distance[j] = hls::sqrt((ap_fixed<64,32>)temp_2);
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
			temp_1[j].real = x_hat[i].real * hls::sqrt((ap_fixed<64,32>)10.0);
			temp_1[j].imag = x_hat[i].imag * hls::sqrt((ap_fixed<64,32>)10.0);
			temp[j].real = temp_1[j].real - _16QAM_Constellation_hw[j].real;
			temp[j].imag = temp_1[j].imag - _16QAM_Constellation_hw[j].imag;
		}
		/*find the closest one*/
		for (j = 0; j < 16; j++)
		{
			temp_2 = (temp[j].real * temp[j].real + temp[j].imag * temp[j].imag);
			distance[j] = hls::sqrt((ap_fixed<64,32>)temp_2);
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
			temp_1[j].real = x_hat[i].real * hls::sqrt((ap_fixed<64,32>)42.0);
			temp_1[j].imag = x_hat[i].imag * hls::sqrt((ap_fixed<64,32>)42.0);
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

unsigned int generate_seed(int sampler_id) {
    // 获取当前时间（秒级精度）
    time_t now = time(0);
    
    // 获取高精度时间（纳秒级）
    auto now_high_res = std::chrono::high_resolution_clock::now();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now_high_res.time_since_epoch()
    ).count();
    
    // 混合多种时间源
    unsigned int seed = static_cast<unsigned int>(now) ^ 
                       static_cast<unsigned int>(nanos) ^ 
                       (static_cast<unsigned int>(nanos >> 32)) ^
                       (sampler_id * 0x9E3779B9);  // 黄金比例常数
    return seed;
}

int main(){
    // ====================== 初始化 FPGA 设备 ======================
    std::cout << "初始化 FPGA 设备, done! \n";
    int device_index = 0;
    std::string xclbin_path = "/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai/hlsKernel/output/MHGD_accel.xclbin";
    float SNR = 25.0;

    auto device = xrt::device(device_index);
    auto uuid = device.load_xclbin(xclbin_path);
    auto krnl = xrt::kernel(device, uuid, "MHGD_detect_accel_hw");

    // ====================== 计算 SNR 相关参数 (sigma2)======================
    const float signal_power = static_cast<float>(Ntr_1) / Ntr_1;
    float sigma2 = (float)Ntr_1 / (float)Ntr_1 * pow(10.0f, -SNR / 10.0f);
    std::cout<<"sigma2 = "<<sigma2<<std::endl;
    std::cout << "计算 SNR 相关参数, done! \n";

    // ====================== 分配设备内存 ======================
    // 根据 HLS 接口的 depth 确定缓冲区大小
    size_t x_size = Ntr_1 * sizeof(Myreal);
    size_t H_single_size = Ntr_1 * Ntr_1 * sizeof(H_real_t);  // 单个H矩阵的实部或虚部大小
    size_t y_single_size = Ntr_1 * sizeof(y_real_t);          // 单个y向量的实部或虚部大小
    size_t v_tb_size = Ntr_1 * iter_1 * sizeof(v_real_t); // depth=256
    std::cout << "计算x, H, y, v_tb的size, done! \n";

    auto bo_x_hat_real = xrt::bo(device, x_size, krnl.group_id(0));//krnl.group_id()
    auto bo_x_hat_imag = xrt::bo(device, x_size, krnl.group_id(1));
    std::cout<<"x内存分配完成!\n";
    auto bo_H_real = xrt::bo(device, H_single_size, krnl.group_id(2));
    auto bo_H_imag = xrt::bo(device, H_single_size, krnl.group_id(3));
    std::cout<<"H内存分配完成!\n";
    auto bo_y_real = xrt::bo(device, y_single_size, krnl.group_id(4));
    auto bo_y_imag = xrt::bo(device, y_single_size, krnl.group_id(5));
    std::cout<<"y内存分配完成!\n";
    auto bo_v_tb_real = xrt::bo(device, v_tb_size, krnl.group_id(6));
    auto bo_v_tb_imag = xrt::bo(device, v_tb_size, krnl.group_id(7));
    auto bo_v_tb_real_2 = xrt::bo(device, v_tb_size, krnl.group_id(8));
    auto bo_v_tb_imag_2 = xrt::bo(device, v_tb_size, krnl.group_id(9));
    auto bo_v_tb_real_3 = xrt::bo(device, v_tb_size, krnl.group_id(10));
    auto bo_v_tb_imag_3 = xrt::bo(device, v_tb_size, krnl.group_id(11));
    auto bo_v_tb_real_4 = xrt::bo(device, v_tb_size, krnl.group_id(12));
    auto bo_v_tb_imag_4 = xrt::bo(device, v_tb_size, krnl.group_id(13));
    std::cout<<"v_tb内存分配完成!\n";
    std::cout << "分配设备内存, done! \n";
    // ====================== 映射主机内存 ======================
    auto x_hat_real_host = bo_x_hat_real.map<Myreal*>();
    auto x_hat_imag_host = bo_x_hat_imag.map<Myimage*>();
    auto H_real_host = bo_H_real.map<H_real_t*>();
    auto H_imag_host = bo_H_imag.map<H_imag_t*>();
    auto y_real_host = bo_y_real.map<y_real_t*>();
    auto y_imag_host = bo_y_imag.map<y_imag_t*>();
    auto v_tb_real_host = bo_v_tb_real.map<v_real_t*>();
    auto v_tb_imag_host = bo_v_tb_imag.map<v_imag_t*>();
    auto v_tb_real_host_2 = bo_v_tb_real_2.map<v_real_t*>();
    auto v_tb_imag_host_2 = bo_v_tb_imag_2.map<v_imag_t*>();
    auto v_tb_real_host_3 = bo_v_tb_real_3.map<v_real_t*>();
    auto v_tb_imag_host_3 = bo_v_tb_imag_3.map<v_imag_t*>();
    auto v_tb_real_host_4 = bo_v_tb_real_4.map<v_real_t*>();
    auto v_tb_imag_host_4 = bo_v_tb_imag_4.map<v_imag_t*>();
    
    
    std::fill(x_hat_real_host, x_hat_real_host + (x_size / sizeof(Myreal)), 0);
    std::fill(x_hat_imag_host, x_hat_imag_host + (x_size / sizeof(Myreal)), 0);
    std::fill(H_real_host, H_real_host + (H_single_size/ sizeof(H_real_t)), 0);
    std::fill(H_imag_host, H_imag_host + (H_single_size/ sizeof(H_imag_t)), 0);
    std::fill(y_real_host, y_real_host + (y_single_size/ sizeof(y_real_t)), 0);
    std::fill(y_imag_host, y_imag_host + (y_single_size/ sizeof(y_imag_t)), 0);
    std::fill(v_tb_real_host, v_tb_real_host + (v_tb_size/ sizeof(v_real_t)), 0);
    std::fill(v_tb_imag_host, v_tb_imag_host + (v_tb_size/ sizeof(v_imag_t)), 0);
    std::fill(v_tb_real_host_2, v_tb_real_host_2 + (v_tb_size/ sizeof(v_real_t)), 0);
    std::fill(v_tb_imag_host_2, v_tb_imag_host_2 + (v_tb_size/ sizeof(v_imag_t)), 0);
    std::fill(v_tb_real_host_3, v_tb_real_host_3 + (v_tb_size/ sizeof(v_real_t)), 0);
    std::fill(v_tb_imag_host_3, v_tb_imag_host_3 + (v_tb_size/ sizeof(v_imag_t)), 0);
    std::fill(v_tb_real_host_4, v_tb_real_host_4 + (v_tb_size/ sizeof(v_real_t)), 0);
    std::fill(v_tb_imag_host_4, v_tb_imag_host_4 + (v_tb_size/ sizeof(v_imag_t)), 0);
    std::cout << "映射主机内存, done! \n";

    // ====================== 文件路径生成 ======================
    char H_file[256], y_file[256], bits_file[256];//, output_file[256];
    snprintf(H_file, sizeof(H_file), H_FILE_TEMPLATE, SNR);
    snprintf(y_file, sizeof(y_file), Y_FILE_TEMPLATE, SNR);
    snprintf(bits_file, sizeof(bits_file), BITS_FILE_TEMPLATE, SNR);
    //snprintf(output_file, sizeof(output_file), OUTPUT_FILE_TEMPLATE, SNR);
    std::cout << "文件路径生成, done! \n";
    std::cout << "H文件:"<<H_file<<"\n";
    std::cout << "y文件:"<<y_file<<"\n";
    std::cout << "原始bit文件:"<<bits_file<<"\n";

    // ====================== 读取输入文件 ======================
    // 定义预存数组
    std::cout<<"定义预存数组\n";
    MyComplex_H input_H[max_iter_1 * Ntr_1 * Ntr_1];
    std::cout<<"定义预存数组H\n";
    MyComplex_y input_y[max_iter_1 * Ntr_1];
    std::cout<<"定义预存数组y\n";
    int origin_bits[Ntr_1 * mu_1 * max_iter_1];
    std::cout<<"定义预存数组origin_bits\n";
    // 读取 H 矩阵（实部+虚部）
    std::cout<<"H文件读取准备begin\n";
    std::ifstream fin_H(H_file);
    std::cout<<"H文件读取begin\n";
    if (!fin_H) throw std::runtime_error("Failed to open H matrix file");
    for (int i = 0; i < max_iter_1 * Ntr_1 * Ntr_1; ++i) {
        fin_H >> input_H[i].real >> input_H[i].imag;
    }
    fin_H.close();
    std::cout << "读取H文件, done! \n";
    // 读取 y 向量
    std::ifstream fin_y(y_file);
    std::cout<<"y文件读取begin\n";
    if (!fin_y) throw std::runtime_error("Failed to open y vector file");
    for (int i = 0; i < max_iter_1 * Ntr_1; ++i) {
        fin_y >> input_y[i].real >> input_y[i].imag;
    }
    fin_y.close();
    std::cout << "读取y文件, done! \n";
    //读取原始bit数据
    std::ifstream fin_bits(bits_file);
    std::cout<<"原始bit文件读取begin\n";
    if (!fin_bits) throw std::runtime_error("Failed to open bits file");
    for (int i = 0; i < Ntr_1 * mu_1 * max_iter_1; ++i) {
        fin_bits >> origin_bits[i];
    }
    fin_bits.close();
    std::cout << "读取原始比特文件, done! \n";
    //读取gauss随机数据
    MyComplex_v v_tb[Ntr_1 * iter_1];
    MyComplex_v v_tb_2[Ntr_1 * iter_1];
    MyComplex_v v_tb_3[Ntr_1 * iter_1];
    MyComplex_v v_tb_4[Ntr_1 * iter_1];
    std::cout<<"高斯随机数据读取begin\n";
    read_gaussian_data_hw_1("/home/ggg_wufuqi/hls/MHGD/gaussian_random_values.txt", v_tb, Ntr_1 * iter_1, 0);
    read_gaussian_data_hw_1("/home/ggg_wufuqi/hls/MHGD/gaussian_random_values.txt", v_tb_2, Ntr_1 * iter_1, 0);
    read_gaussian_data_hw_1("/home/ggg_wufuqi/hls/MHGD/gaussian_random_values.txt", v_tb_3, Ntr_1 * iter_1, 0);
    read_gaussian_data_hw_1("/home/ggg_wufuqi/hls/MHGD/gaussian_random_values.txt", v_tb_4, Ntr_1 * iter_1, 0);
    for (int i = 0; i < Ntr_1 * iter_1; ++i) {
        v_tb_real_host[i] = v_tb[i].real;
        v_tb_imag_host[i] = v_tb[i].imag;
    }
    for (int i = 0; i < Ntr_1 * iter_1; ++i) {
        v_tb_real_host_2[i] = v_tb_2[i].real;
        v_tb_imag_host_2[i] = v_tb_2[i].imag;
    }
    for (int i = 0; i < Ntr_1 * iter_1; ++i) {
        v_tb_real_host_3[i] = v_tb_3[i].real;
        v_tb_imag_host_3[i] = v_tb_3[i].imag;
    }
    for (int i = 0; i < Ntr_1 * iter_1; ++i) {
        v_tb_real_host_4[i] = v_tb_4[i].real;
        v_tb_imag_host_4[i] = v_tb_4[i].imag;
    }
    std::cout << "读取gauss随机数据文件, done! \n";
    // ====================== 同步数据到设备 ======================
    bo_x_hat_real.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_x_hat_imag.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_H_real.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_H_imag.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_y_real.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_y_imag.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_v_tb_real.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_v_tb_imag.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_v_tb_real_2.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_v_tb_imag_2.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_v_tb_real_3.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_v_tb_imag_3.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_v_tb_real_4.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_v_tb_imag_4.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    std::cout << "同步数据到设备, done! \n";
    
    // ====================== 执行内核 ======================
    int total_error_bits = 0;
    int total_bits = 0;
    //std::ofstream fout(output_file);
    //if (!fout) throw std::runtime_error("Failed to create output file");
    std::cout << "准备进入for循环! \n";
    for (int iter = 0; iter < max_iter_1; iter++) {
        // 定义当前迭代的H矩阵以及y向量
        H_real_t current_H_real[Ntr_1 * Ntr_1];
        H_imag_t current_H_imag[Ntr_1 * Ntr_1];
        y_real_t current_y_real[Ntr_1];
        y_imag_t current_y_imag[Ntr_1];
        int current_bits[Ntr_1 * mu_1];
        // 设置当前迭代的输入数据偏移
        const int H_offset = iter * Ntr_1 * Ntr_1;
        const int y_offset = iter * Ntr_1;
        const int bits_offset = iter * Ntr_1 * mu_1;
        // H和y分离实部/虚部地读取，originbits直接读取
        for (int j = 0; j < Ntr_1 * Ntr_1; ++j) {
            current_H_real[j] = input_H[H_offset + j].real;
            current_H_imag[j] = input_H[H_offset + j].imag;
        }
        for (int j = 0; j < Ntr_1; ++j) {
            current_y_real[j] = input_y[y_offset + j].real;
            current_y_imag[j] = input_y[y_offset + j].imag;
        }
        for (int j = 0; j < Ntr_1 * mu_1; ++j) {
            current_bits[j] = origin_bits[j + bits_offset];
        }

        // 写入H的实部和虚部
        void* bo_H_real_ptr = bo_H_real.map<void*>(); // 映射到主机指针
        std::memcpy(bo_H_real_ptr, current_H_real, H_single_size); // 拷贝数据
        void* bo_H_imag_ptr = bo_H_imag.map<void*>();
        std::memcpy(bo_H_imag_ptr, current_H_imag, H_single_size);
        // 写入y的实部和虚部
        void* bo_y_real_ptr = bo_y_real.map<void*>();
        std::memcpy(bo_y_real_ptr, current_y_real, y_single_size);
        void* bo_y_imag_ptr = bo_y_imag.map<void*>();
        std::memcpy(bo_y_imag_ptr, current_y_imag, y_single_size);

        // // [!] 打印输入数据
        // std::cout << "\n\n[DEBUG] ===== Iteration " << iter << " Input Data =====\n";
        // // 打印H矩阵 (仅打印前2x2部分)
        // std::cout << "H Matrix (first 2x2 elements):\n";
        // for (int i = 0; i < 2; ++i) {
        //     for (int j = 0; j < 2; ++j) {
        //         int idx = i * Ntr_1 + j;
        //         std::cout << "H[" << i << "][" << j << "] = (" 
        //                   << current_H_real[idx] << " + " 
        //                   << current_H_imag[idx] << "j)\t";
        //     }
        //     std::cout << std::endl;
        // }

        // // 打印y向量 (前5个元素)
        // std::cout << "\ny Vector (first 5 elements):\n";
        // for (int i = 0; i < 5; ++i) {
        //     std::cout << "y[" << i << "] = (" 
        //               << current_y_real[i] << " + " 
        //               << current_y_imag[i] << "j)\n";
        // }

        // // 打印v_tb (前5个元素)
        // std::cout << "\nv_tb (first 5 elements):\n";
        // for (int i = 0; i < 5; ++i) {
        //     std::cout << "v_tb[" << i << "] = (" 
        //               << v_tb_real_host[i] << " + " 
        //               << v_tb_imag_host[i] << "j)\n";
        // }

        // // 打印origin_bits (前5个元素)
        // std::cout << "\norigin_bits (first 5 elements):\n";
        // for (int i = 0; i < 5; ++i) {
        //     std::cout << "origin_bits[" << i << "] = " 
        //               << current_bits[i] << "\n";
        // }

        // // 打印sigma2
        // std::cout << "\nsigma2 = " << sigma2 << std::endl;
        /*随机种子产生*/
        unsigned int seed[samplers];
        for (int i = 0; i < samplers; i++) {
           seed[i] = generate_seed(i);
           std::this_thread::sleep_for(std::chrono::microseconds(100 * (i + 1)));
        }

        //同步数据到设备
        bo_H_real.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        bo_H_imag.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        bo_y_real.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        bo_y_imag.sync(XCL_BO_SYNC_BO_TO_DEVICE);

        // 记录开始时间点
        auto start_time = std::chrono::high_resolution_clock::now();

        // 执行内核
        auto run = krnl(bo_x_hat_real,          // group_id(0)
                        bo_x_hat_imag,          // group_id(1)
                        bo_H_real,              // group_id(2)
                        bo_H_imag,              // group_id(3)
                        bo_y_real,              // group_id(4)
                        bo_y_imag,              // group_id(5)
                        bo_v_tb_real,           // group_id(6)
                        bo_v_tb_imag,           // group_id(7)
                        bo_v_tb_real_2,           // group_id(6)
                        bo_v_tb_imag_2,           // group_id(7)
                        bo_v_tb_real_3,           // group_id(6)
                        bo_v_tb_imag_3,           // group_id(7)
                        bo_v_tb_real_4,           // group_id(6)
                        bo_v_tb_imag_4,           // group_id(7)
                        sigma2,                  // 标量参数
                        seed[0],
                        seed[1],
                        seed[2],
                        seed[3]
        );
        run.wait();
        // 计算耗时
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        // 打印耗时（保留3位小数）
        std::cout << "[Perf] Iter " << iter 
        << " Kernel Time: " 
        << std::fixed << std::setprecision(3) 
        << duration.count() / 1000.0  // 转换为毫秒
        << " ms\n";
        std::cout << "(for)内核结束第" << iter + 1 << "次结束!\n";
        // 回读结果
        bo_x_hat_real.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        bo_x_hat_imag.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

        std::cout << "\n[DEBUG] ===== Pre-Demodulation Data (Iter " << iter << ") =====" << std::endl;
        // 解调
        int bits_demod[Ntr_1 * mu_1];
        MyComplex x_hat[8];
	    for(int i=0; i<8; i++){
	    	x_hat[i].real = x_hat_real_host[i];
	    	x_hat[i].imag = x_hat_imag_host[i];
	    }
        // 打印x_hat前5个元素的实部虚部
        std::cout << "x_hat_real_host (first 5 elements):" << std::endl;
        for (int i = 0; i < 5 && i < Ntr_1; ++i) {
            std::cout << "x_hat_real[" << i << "] = " << x_hat[i].real 
                      << std::endl;
        }

        std::cout << "x_hat_imag_host (first 5 elements):" << std::endl;
        for (int i = 0; i < 5 && i < Ntr_1; ++i) {
            std::cout << "x_hat_imag[" << i << "] = " << x_hat[i].imag
                      << std::endl;
        }
        QAM_Demodulation_hw_de(x_hat, Ntr_1, mu_1, bits_demod);
        std::cout << "(for)解调结束第" << iter + 1 << "次结束!\n";
        // ============== 新增：打印解调结果 ==============
        //const int print_count = 20; // 打印前20个比特
        //std::cout << "\n[DEMOD DEBUG] === Iter " << iter << " Demod Result ===" << std::endl;
        //std::cout << "First " << print_count << " bits: [";
        //for (int i = 0; i < print_count; ++i) {
        //    if (i >= Ntr_1 * mu_1) break; // 安全边界检查

        //    // 标注非法值（非0/1）
        //    const int bit = bits_demod[i];
        //    if (bit != 0 && bit != 1) {
        //        std::cout << "\033[31m" << bit << "\033[0m"; // 红色显示非法值
        //    } else {
        //        std::cout << bit;
        //    }

        //    if (i != print_count - 1) std::cout << ", ";
        //}
        //std::cout << "]" << std::endl;

        //// 写入解调结果
        //for (int i = 0; i < Ntr_1 * mu_1; ++i) {
        //    fout << bits_demod[i] << std::endl;
        //}

        // 计算误码率
        int error_bits = 0;
        error_bits = unequal_times_hw(bits_demod, current_bits, Ntr_1 * mu_1);
        total_error_bits += error_bits;
        total_bits += mu_1 * Ntr_1;
        std::cout << "Iter " << iter + 1 << "/" << max_iter_1 
                  << ", Errors: " << error_bits 
                  << ", Total Errors: " << total_error_bits << std::endl;
    }

    // ====================== 计算最终 BER ======================
    const float BER = (float)total_error_bits / (float)total_bits;
    std::cout << "\nFinal Result - SNR: " << SNR 
              << ", BER: " << BER << std::endl;

    return 0;
}