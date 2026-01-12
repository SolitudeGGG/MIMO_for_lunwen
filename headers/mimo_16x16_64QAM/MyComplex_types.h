#pragma once
#include "ap_fixed.h"	//ap_fixed<18,6,AP_TRN_ZERO,AP_SAT>		<W,I,Q,O,N>
#include "ap_int.h"	//ap_int<N> or ap_uint<N>, 1<=N<=1024
#include "hls_math.h"	//data_t s = hls::sinf(angle);
#include "hls_stream.h"

// MIMO Configuration: 16x16 with 64QAM at SNR25
// Optimized ap_fixed types for 16x16 MIMO system with 64QAM modulation

/*优化后的数据类型 - 16x16 64QAM SNR25*/
// 变量: step_size
typedef ap_fixed<22, 5> step_size_t;

// 变量: r_norm
typedef ap_fixed<22, 11> r_norm_t;

// 变量: r_norm_survivor
typedef ap_fixed<8, 6> r_norm_survivor_t;

// 变量: lr
typedef ap_fixed<20, 5> lr_t;

// 变量: r_norm_prop
typedef ap_fixed<21, 11> r_norm_prop_t;

// 变量: z_grad_real
typedef ap_fixed<26, 6> z_grad_real_t;

// 变量: z_grad_imag
typedef ap_fixed<26, 6> z_grad_imag_t;

// 变量: z_prop_real
typedef ap_fixed<25, 8> z_prop_real_t;

// 变量: z_prop_imag
typedef ap_fixed<25, 8> z_prop_imag_t;

// 变量: x_prop_real
typedef ap_fixed<52, 12> x_prop_real_t;

// 变量: x_prop_imag
typedef ap_fixed<52, 12> x_prop_imag_t;

// 变量: H_real
typedef ap_fixed<24, 6> H_real_t;

// 变量: H_imag
typedef ap_fixed<24, 6> H_imag_t;

// 变量: y_real
typedef ap_fixed<24, 6> y_real_t;

// 变量: y_imag
typedef ap_fixed<24, 6> y_imag_t;

// 变量: v_real
typedef ap_fixed<24, 6> v_real_t;

// 变量: v_imag
typedef ap_fixed<24, 6> v_imag_t;

typedef struct {
    y_real_t real;  // 实部
    y_imag_t imag;  // 虚部
} MyComplex_y;

typedef struct {
    H_real_t real;  // 实部
    H_imag_t imag;  // 虚部
} MyComplex_H;

typedef struct {
    z_grad_real_t real;  // 实部
    z_grad_imag_t imag;  // 虚部
} MyComplex_z_grad;

typedef struct {
    z_prop_real_t real;  // 实部
    z_prop_imag_t imag;  // 虚部
} MyComplex_z_prop;

typedef struct {
    x_prop_real_t real;  // 实部
    x_prop_imag_t imag;  // 虚部
} MyComplex_x_prop;

typedef struct {
    v_real_t real;  // 实部
    v_imag_t imag;  // 虚部
} MyComplex_v;

// 变量: HH_H_real
typedef ap_fixed<30, 6> HH_H_real_t;

// 变量: HH_H_imag
typedef ap_fixed<30, 6> HH_H_imag_t;

// 变量: grad_preconditioner_real
typedef ap_fixed<30, 11> grad_preconditioner_real_t;

// 变量: grad_preconditioner_imag
typedef ap_fixed<30, 11> grad_preconditioner_imag_t;

// 变量: x_mmse_real
typedef ap_fixed<52, 12> x_mmse_real_t;

// 变量: x_mmse_imag
typedef ap_fixed<52, 12> x_mmse_imag_t;

// 变量: pmat_real
typedef ap_fixed<22, 6> pmat_real_t;

// 变量: pmat_imag
typedef ap_fixed<22, 6> pmat_imag_t;

// 变量: r_real
typedef ap_fixed<21, 6> r_real_t;

// 变量: r_imag
typedef ap_fixed<21, 6> r_imag_t;

// 变量: pr_prev_real
typedef ap_fixed<20, 6> pr_prev_real_t;

// 变量: pr_prev_imag
typedef ap_fixed<20, 6> pr_prev_imag_t;

// 变量: temp_NtNt_real
typedef ap_fixed<52, 12> temp_NtNt_real_t;

// 变量: temp_NtNt_imag
typedef ap_fixed<52, 12> temp_NtNt_imag_t;

// 变量: temp_NtNr_real
typedef ap_fixed<22, 6> temp_NtNr_real_t;

// 变量: temp_NtNr_imag
typedef ap_fixed<22, 6> temp_NtNr_imag_t;

// 变量: temp_1_real
typedef ap_fixed<22, 11> temp_1_real_t;

// 变量: temp_1_imag
typedef ap_fixed<2, 1> temp_1_imag_t;

// 变量: _temp_1_real
typedef ap_fixed<24, 11> _temp_1_real_t;

// 变量: _temp_1_imag
typedef ap_fixed<2, 1> _temp_1_imag_t;

// 变量: temp_Nt_real
typedef ap_fixed<30, 6> temp_Nt_real_t;

// 变量: temp_Nt_imag
typedef ap_fixed<30, 6> temp_Nt_imag_t;

// 变量: temp_Nr_real
typedef ap_fixed<22, 6> temp_Nr_real_t;

// 变量: temp_Nr_imag
typedef ap_fixed<22, 6> temp_Nr_imag_t;

// 变量: sigma2eye_real
typedef ap_fixed<16, 6> sigma2eye_real_t;

// 变量: sigma2eye_imag
typedef ap_fixed<16, 6> sigma2eye_imag_t;

// 变量: local_temp_1
typedef ap_fixed<52, 12> local_temp_1_t;

// 变量: local_temp_2
typedef ap_fixed<52, 12> local_temp_2_t;

typedef struct {
    HH_H_real_t real;  // 实部
    HH_H_imag_t imag;  // 虚部
} MyComplex_HH;

typedef struct {
    grad_preconditioner_real_t real;  // 实部
    grad_preconditioner_imag_t imag;  // 虚部
} MyComplex_grad_preconditioner;

typedef struct {
    x_mmse_real_t real;  // 实部
    x_mmse_imag_t imag;  // 虚部
} MyComplex_x_mmse;

typedef struct {
    pmat_real_t real;  // 实部
    pmat_imag_t imag;  // 虚部
} MyComplex_pmat;

typedef struct {
    r_real_t real;  // 实部
    r_imag_t imag;  // 虚部
} MyComplex_r;

typedef struct {
    pr_prev_real_t real;  // 实部
    pr_prev_imag_t imag;  // 虚部
} MyComplex_pr_prev;

typedef struct {
    temp_NtNt_real_t real;  // 实部
    temp_NtNt_imag_t imag;  // 虚部
} MyComplex_temp_NtNt;

typedef struct {
    temp_NtNr_real_t real;  // 实部
    temp_NtNr_imag_t imag;  // 虚部
} MyComplex_temp_NtNr;

typedef struct {
    temp_1_real_t real;  // 实部
    temp_1_imag_t imag;  // 虚部
} MyComplex_temp_1;

typedef struct {
    _temp_1_real_t real;  // 实部
    _temp_1_imag_t imag;  // 虚部
} MyComplex__temp_1;

typedef struct {
    temp_Nt_real_t real;  // 实部
    temp_Nt_imag_t imag;  // 虚部
} MyComplex_temp_Nt;

typedef struct {
    temp_Nr_real_t real;  // 实部
    temp_Nr_imag_t imag;  // 虚部
} MyComplex_temp_Nr;

typedef struct {
    sigma2eye_real_t real;  // 实部
    sigma2eye_imag_t imag;  // 虚部
} MyComplex_sigma2eye;

typedef ap_fixed<52,12> like_float;
typedef ap_fixed<52,12> Myreal;
typedef ap_fixed<52,12> Myimage;

// 自定义复数结构体
typedef struct {
    Myreal real;  // 实部
    Myimage imag;  // 虚部
} MyComplex;

typedef struct {
    float real;  // 实部
    float imag;  // 虚部
} MyComplex_f;
