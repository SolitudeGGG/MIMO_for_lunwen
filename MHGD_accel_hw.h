#pragma once
#include "MyComplex_1.h"
#include "hls_math.h"

// #ifdef SENSITIVITY_ANALYSIS_MODE
// #include "/home/ggg_wufuqi/hls/MIMO_detect-main/mimo_cpp_gai/sensitivity_types.hpp"

// // 为关键变量使用灵敏度分析专用类型
// #define SENSITIVITY_TYPE(VAR) VAR##_t
// #else
// // 非分析模式使用标准类型
// #define SENSITIVITY_TYPE(VAR) ap_fixed<40,8>
// #endif

typedef struct _IO_FILE FILE;

#define LCG_A 1664525
#define LCG_C 1013904223
#define LIMIT_MAX 0x7fffffff
#define LIMIT_MAX_F 2147483647.0f
// 巴雷特约减预计算参数（适用于模数m=2^31-1）
#define BARRETT_K 33                  // 选择k=33满足2^k > m
#define BARRETT_MU ((1ULL<<33)/0x7FFFFFFF + 1) // 预计算μ=ceil(2^k/m)

#define EPS like_float(1e-9)  // 根据定点数精度调整

#define M_PI 3.14159265358979323846
#define IDX(i, j) (i*8 + j)
#define MAX_VECTOR_LEN (Ntr_1 - 1)

// PCG32参数（硬件友好型）
const uint64_t PCG_MULTIPLIER = 6364136223846793005ULL;
const uint64_t PCG_INCREMENT = 1442695040888963407ULL;  //奇数确保全周期

/*算法参数*/
static const int Ntr_1 = 8;/*发射&接收天线数*/
static const int Ntr_2 = Ntr_1*Ntr_1;/*NtorNr^2*/
static const int iter_1 = 10;/*采样器的采样数*/
static const int num_ran = iter_1*Ntr_1;/*需要的高斯随机噪声数*/
static const int mu_1 = 4;/*调制阶数*/
static const int mu_double = 16;/*调制阶数对应的2指数值*/
static const int mmse_init_1 = 0;/*是否使用MMSE检测的结果作为MCMC采样的初始值*/
static const int mmse_init_2 = 0;
static const int lr_approx_1 = 0;
static const int lr_approx_2 = 0;
static const int max_iter_1 = 10;/*希望仿真的最大轮数*/
static const int samplers = 4;

void read_gaussian_data_hw(const char* filename, MyComplex_v* array, int n, int offset);
void QAM_Demodulation_hw(MyComplex* x_hat, int Nt, int mu, int* bits_demod);
void QPSK_Demodulation_hw(MyComplex* x_hat, int Nt, int* bits_demod);
void _16QAM_Demodulation_hw(MyComplex* x_hat, int Nt, int* bits_demod);void _64QAM_Demodulation_hw(MyComplex* x_hat, int Nt, int* bits_demod);
int argmin_hw(like_float* array, int n);
int unequal_times_hw(int* array1, int* array2, int n);

unsigned int lcg_rand_hw();
like_float lcg_rand_1_hw();
Myreal complex_norm_sqr(MyComplex a);
void get_dqam_hw(like_float &dqam);
template <typename T>void c_eye_generate_hw(T* Mat, float val);
template <typename TH, typename TY, typename TV, typename TH_real, typename TH_imag, typename TY_real, typename TY_imag, typename TV_real, typename TV_imag>
void data_local(TH* H, TY* y, TV* v_tb, TH_real* H_real, TH_imag* H_imag, TY_real* y_real, TY_imag* y_imag, TV_real* v_tb_real, TV_imag* v_tb_imag);
template<typename TA, typename TB, typename TR>
void c_matmultiple_hw(TA* matA, int transA, TB* matB, int transB, int ma, int na, int mb, int nb, TR* res);
template<typename TA, typename TB, typename TR>
void my_complex_add_hw(const TA a[], const TB b[], TR r[]);
template<typename TA, typename TB, typename TR>
void my_complex_add_hw_1(const TA a[], const TB b[], TR r[]);
template<typename TA>
void initMatrix_hw(TA* A);
template<typename TA, typename TB>
MyComplex complex_divide_hw(TA a, TB b);
template<typename TA, typename TB>
MyComplex complex_multiply_hw(TA a, TB b);
template<typename TA, typename TB>
MyComplex complex_add_hw(TA a, TB b);
template<typename TA, typename TB>
MyComplex complex_subtract_hw(TA a, TB b);
template<typename TA, typename TB, typename TR>
void my_complex_sub_hw(const TA a[], const TB b[], TR r[]);
template<typename TA, typename TB, typename TC>
void MulMatrix_hw(const TA* A, const TB* B, TC* C);
template<typename TA>
void Inverse_LU_hw(TA* A);
template<typename TX>
void my_complex_scal_hw(const like_float alpha, TX* X, const int incX);
template<typename TX>
void my_complex_scal_hw_1(const like_float alpha, TX* X, const int incX);
template<typename TX, typename TX_hat, typename x_real, typename hat_real>
void map_hw(like_float dqam, TX* x, TX_hat* x_hat);
void generateUniformRandoms_int_hw(int* x_init);
template<typename TX, typename TY>
void my_complex_copy_hw(const TX* X, const int incX, TY* Y, const int incY);
template<typename TX, typename TY>
void my_complex_copy_hw_1(const TX* X, const int incX, TY* Y, const int incY);
void generateUniformRandoms_float_hw(like_float* p_uni);
template<typename TH, typename T_grad, typename T_pat>
void learning_rate_line_search_hw(int lr_approx, TH* H, T_grad* grad_preconditioner, int Nr, int Nt, T_pat* pmat);
void r_hw(MyComplex* H, int transB, int transA, MyComplex* x_hat, int Nr, int Nt, MyComplex* r, MyComplex* y);
void r_cal_hw(MyComplex* r, int transA, int transB, int Nr, int Nt, MyComplex* temp_1, MyComplex* x_hat, MyComplex* x_survivor, like_float r_norm, like_float r_norm_survivor);
void z_grad_hw(MyComplex* H, int transA, int transB, MyComplex* temp_Nt, MyComplex* grad_preconditioner, MyComplex* z_grad, like_float lr, MyComplex* x_hat);
void gauss_add_hw(MyComplex* v, MyComplex* v_tb, int offset, like_float step_size, MyComplex* z_grad, MyComplex* z_prop);
void r_newnorm_hw(MyComplex* H, int transB, MyComplex* x_prop, int transA, MyComplex* temp_Nr, MyComplex* y, MyComplex* r_prop, MyComplex* temp_1, like_float r_norm_prop);
void survivor_hw(like_float r_norm_survivor, like_float r_norm_prop, MyComplex* x_prop, MyComplex* x_survivor);
void acceptance_hw(int transB, int transA, like_float r_norm_prop, like_float r_norm, like_float log_pacc, like_float p_acc, like_float* p_uni, MyComplex* x_prop , MyComplex* x_hat, MyComplex* r_prop, MyComplex* r, MyComplex* pmat, MyComplex* pr_prev, MyComplex* temp_1, MyComplex* _temp_1, like_float lr, like_float step_size, like_float dqam, like_float alpha);
template<typename TX, typename TY_real, typename TY_imag>
void out_hw(const TX* X, const int incX, TY_real* Y_real, TY_imag* Y_imag, int incY);
template <typename T>
T fixed_floor(const T& val);

void Inverse_LU(MyComplex_f* A);
void initMatrix(MyComplex_f* A);
void MulMatrix(const MyComplex_f* A, const MyComplex_f* B, MyComplex_f* C);
MyComplex_f complex_divide(MyComplex_f a, MyComplex_f b);
MyComplex_f complex_multiply(MyComplex_f a, MyComplex_f b);
MyComplex_f complex_add(MyComplex_f a, MyComplex_f b);
MyComplex_f complex_subtract(MyComplex_f a, MyComplex_f b);
// MyComplex_f complex_divide(MyComplex_f a, MyComplex_f b);

//hardware function ggoodd
unsigned int fast_mod_barrett(uint64_t a);
template <unsigned int INIT_SEED = 1>
unsigned int lcg_rand_hw_opt(unsigned int &seed);
like_float lcg_rand_1_hw_fixed(unsigned int &seed);
void generateUniformRandoms_int_hw_pro_0(unsigned int &seed, int* x_init);
void generateUniformRandoms_float_hw_pro(unsigned int &seed, like_float* p_uni);
template<typename TA, typename TB, typename TR>
void c_matmultiple_hw_pro(
    TA* matA, int transA,
    TB* matB, int transB,
    int ma, int na, int mb, int nb,
    TR* res);

// 黑盒版本复数乘法函数
void c_matmultiple_hw_pro_wrapper(
    ap_fixed<40,8>* matA_real, ap_fixed<40,8>* matA_imag,
    ap_fixed<40,8>* matB_real, ap_fixed<40,8>* matB_imag,
    int transA, int transB,
    int ma, int na, int mb, int nb,
    ap_fixed<40,8>* res_real, ap_fixed<40,8>* res_imag);




void constellation_norm_initial(
	MyComplex* constellation_norm, like_float dqam
);
void grad_preconditioner_updater_hw(
	MyComplex_H* H, MyComplex_HH* HH_H, MyComplex_sigma2eye* sigma2eye, 
	MyComplex_grad_preconditioner* grad_preconditioner, float sigma2_local, like_float dqam
);
void get_alpha(like_float &alpha);
void x_initialize_hw(
	int mmse_init, MyComplex_sigma2eye* sigma2eye, int Nt, int Nr, float sigma2, MyComplex_HH* HH_H, 
	MyComplex_H* H, MyComplex_y* y, int num,
	like_float dqam, MyComplex* x_hat, MyComplex* constellation_norm, unsigned int& seed
);
void r_hw(MyComplex_H* H, MyComplex* x_hat, MyComplex_r* r, MyComplex_y* y);
void r_cal_hw(MyComplex_r* r, MyComplex* x_hat, MyComplex* x_survivor, r_norm_t &r_norm, r_norm_t &r_norm_survivor);
void lr_hw(int lr_approx, MyComplex_pmat* pmat, MyComplex_r* r, MyComplex_pr_prev* pr_prev, lr_t &lr, int num);
void step_size_hw(step_size_t &step_size, like_float alpha, like_float dqam, r_norm_t r_norm);
void data_copy(
	/*静态量*/
	MyComplex_H H_local[Ntr_2], MyComplex_y y_local[Ntr_1], MyComplex_v v_tb_local[Ntr_1 * iter_1], MyComplex_grad_preconditioner grad_preconditioner[Ntr_2],
	MyComplex_pmat pmat[Ntr_2], MyComplex constellation_norm[mu_double], like_float dqam, like_float alpha, MyComplex_sigma2eye sigma2eye[Ntr_2], MyComplex_HH HH_H[64],
	/*copy结果*/
	MyComplex_H* H_local_2, MyComplex_y* y_local_2, MyComplex_v* v_tb_local_2, MyComplex_grad_preconditioner* grad_preconditioner_2,
	MyComplex_pmat* pmat_2, MyComplex* constellation_norm_2, like_float &dqam_2, like_float &alpha_2, MyComplex_sigma2eye* sigma2eye_2, MyComplex_HH* HH_H_2
);
void samplers_process(
	/*静态量*/
	MyComplex_H H_local[Ntr_2], MyComplex_y y_local[Ntr_1], MyComplex_v v_tb_local[Ntr_1 * iter_1], MyComplex_grad_preconditioner grad_preconditioner[Ntr_2],
	MyComplex_pmat pmat[Ntr_2], MyComplex constellation_norm[16], like_float dqam, like_float alpha, float sigma2_local, int lr_approx, int num,
	/*动态*/
	MyComplex* x_hat, MyComplex_r* r, r_norm_t &r_norm, MyComplex_pr_prev* pr_prev, lr_t &lr,
	step_size_t &step_size, int &offset, MyComplex* x_survivor, r_norm_t &r_norm_survivor, unsigned int& seed
);
void comparison_r(
	/*静态量*/
	r_norm_t r_norm_survivor_all[samplers],
	MyComplex x_survivor_all[samplers][Ntr_1],
	/*结果量*/
	MyComplex* x_survivor_final
);


void comparison_r_wrapper(
    hls::stream<r_norm_t> r_norm_in[samplers],
    hls::stream<Myreal> x_real_in[samplers],
    hls::stream<Myimage> x_imag_in[samplers],
    MyComplex* x_final
);
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
);


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
);


/////////////////////////////////////////////////////////////////////////////
