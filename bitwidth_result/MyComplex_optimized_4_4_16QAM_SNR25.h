// 扩展的灵敏度分析类型定义 - 包含子函数局部变量
#pragma once
#include <ap_fixed.h>

// ====== 主要变量 (来自MyComplex_1.h) ======

// 变量: step_size
typedef ap_fixed<6, 4> step_size_t;

// 变量: r_norm
typedef ap_fixed<10, 8> r_norm_t;

// 变量: r_norm_survivor
typedef ap_fixed<6, 4> r_norm_survivor_t;

// 变量: lr
typedef ap_fixed<6, 4> lr_t;

// 变量: r_norm_prop
typedef ap_fixed<10, 8> r_norm_prop_t;

// 变量: z_grad_real
typedef ap_fixed<6, 4> z_grad_real_t;

// 变量: z_grad_imag
typedef ap_fixed<6, 4> z_grad_imag_t;

// 变量: z_prop_real
typedef ap_fixed<8, 6> z_prop_real_t;

// 变量: z_prop_imag
typedef ap_fixed<8, 6> z_prop_imag_t;

// 变量: x_prop_real
typedef ap_fixed<10, 8> x_prop_real_t;

// 变量: x_prop_imag
typedef ap_fixed<10, 8> x_prop_imag_t;

// 变量: H_real
typedef ap_fixed<6, 4> H_real_t;

// 变量: H_imag
typedef ap_fixed<6, 4> H_imag_t;

// 变量: y_real
typedef ap_fixed<6, 4> y_real_t;

// 变量: y_imag
typedef ap_fixed<6, 4> y_imag_t;

// 变量: v_real
typedef ap_fixed<6, 4> v_real_t;

// 变量: v_imag
typedef ap_fixed<6, 4> v_imag_t;

// 变量: HH_H_real
typedef ap_fixed<6, 4> HH_H_real_t;

// 变量: HH_H_imag
typedef ap_fixed<6, 4> HH_H_imag_t;

// 变量: grad_preconditioner_real
typedef ap_fixed<10, 8> grad_preconditioner_real_t;

// 变量: grad_preconditioner_imag
typedef ap_fixed<10, 8> grad_preconditioner_imag_t;

// 变量: x_mmse_real
typedef ap_fixed<10, 8> x_mmse_real_t;

// 变量: x_mmse_imag
typedef ap_fixed<10, 8> x_mmse_imag_t;

// 变量: pmat_real
typedef ap_fixed<6, 4> pmat_real_t;

// 变量: pmat_imag
typedef ap_fixed<6, 4> pmat_imag_t;

// 变量: r_real
typedef ap_fixed<6, 4> r_real_t;

// 变量: r_imag
typedef ap_fixed<6, 4> r_imag_t;

// 变量: pr_prev_real
typedef ap_fixed<6, 4> pr_prev_real_t;

// 变量: pr_prev_imag
typedef ap_fixed<6, 4> pr_prev_imag_t;

// 变量: temp_NtNt_real
typedef ap_fixed<10, 8> temp_NtNt_real_t;

// 变量: temp_NtNt_imag
typedef ap_fixed<10, 8> temp_NtNt_imag_t;

// 变量: temp_NtNr_real
typedef ap_fixed<6, 4> temp_NtNr_real_t;

// 变量: temp_NtNr_imag
typedef ap_fixed<6, 4> temp_NtNr_imag_t;

// 变量: temp_1_real
typedef ap_fixed<10, 8> temp_1_real_t;

// 变量: temp_1_imag
typedef ap_fixed<4, 1> temp_1_imag_t;

// 变量: _temp_1_real
typedef ap_fixed<10, 8> _temp_1_real_t;

// 变量: _temp_1_imag
typedef ap_fixed<4, 1> _temp_1_imag_t;

// 变量: temp_Nt_real
typedef ap_fixed<6, 4> temp_Nt_real_t;

// 变量: temp_Nt_imag
typedef ap_fixed<6, 4> temp_Nt_imag_t;

// 变量: temp_Nr_real
typedef ap_fixed<6, 4> temp_Nr_real_t;

// 变量: temp_Nr_imag
typedef ap_fixed<6, 4> temp_Nr_imag_t;

// 变量: sigma2eye_real
typedef ap_fixed<6, 4> sigma2eye_real_t;

// 变量: sigma2eye_imag
typedef ap_fixed<6, 4> sigma2eye_imag_t;

// 变量: local_temp_1
typedef ap_fixed<10, 8> local_temp_1_t;

// 变量: local_temp_2
typedef ap_fixed<10, 8> local_temp_2_t;


// ====== 子函数局部变量 ======
// i: 默认配置
typedef ap_fixed<24, 8> i_local_t;
// j: 默认配置
typedef ap_fixed<24, 8> j_local_t;
// best_id: 默认配置
typedef ap_fixed<24, 8> best_id_local_t;
// int_real: 默认配置
typedef ap_fixed<24, 8> int_real_local_t;
// int_imag: 默认配置
typedef ap_fixed<24, 8> int_imag_local_t;
// unequal: 默认配置
typedef ap_fixed<24, 8> unequal_local_t;
// lcg_seed: 默认配置
typedef ap_fixed<24, 8> lcg_seed_local_t;
// l: 默认配置
typedef ap_fixed<24, 8> l_local_t;
// W: 默认配置
typedef ap_fixed<24, 8> W_local_t;
// I: 默认配置
typedef ap_fixed<24, 8> I_local_t;
// k: 默认配置
typedef ap_fixed<24, 8> k_local_t;
// rand_int: 默认配置
typedef ap_fixed<24, 8> rand_int_local_t;
// m: 默认配置
typedef ap_fixed<24, 8> m_local_t;
// n: 默认配置
typedef ap_fixed<24, 8> n_local_t;
// ROW_REG_DEPTH: 默认配置
typedef ap_fixed<24, 8> ROW_REG_DEPTH_local_t;
// COL_REG_DEPTH: 默认配置
typedef ap_fixed<24, 8> COL_REG_DEPTH_local_t;
// MAX_DEPTH: 默认配置
typedef ap_fixed<24, 8> MAX_DEPTH_local_t;
// flag: 默认配置
typedef ap_fixed<24, 8> flag_local_t;
// r: 默认配置
typedef ap_fixed<24, 8> r_local_t;
// cycle: 默认配置
typedef ap_fixed<24, 8> cycle_local_t;
// row: 默认配置
typedef ap_fixed<24, 8> row_local_t;
// col: 默认配置
typedef ap_fixed<24, 8> col_local_t;
// j_prime: 默认配置
typedef ap_fixed<24, 8> j_prime_local_t;
// transA: 默认配置
typedef ap_fixed<24, 8> transA_local_t;
// transB: 默认配置
typedef ap_fixed<24, 8> transB_local_t;
// choice: 默认配置
typedef ap_fixed<24, 8> choice_local_t;
// offset: 默认配置
typedef ap_fixed<24, 8> offset_local_t;
// lr_approx: 学习率/步长，小数值
typedef ap_fixed<18, 4> lr_approx_local_t;
// mmse_init: 默认配置
typedef ap_fixed<24, 8> mmse_init_local_t;
// seed: 默认配置
typedef ap_fixed<24, 8> seed_local_t;
// sampler_id_1: 默认配置
typedef ap_fixed<24, 8> sampler_id_1_local_t;
// sampler_id_2: 默认配置
typedef ap_fixed<24, 8> sampler_id_2_local_t;
// sampler_id_3: 默认配置
typedef ap_fixed<24, 8> sampler_id_3_local_t;
// sampler_id_4: 默认配置
typedef ap_fixed<24, 8> sampler_id_4_local_t;
// sampler_id_5: 默认配置
typedef ap_fixed<24, 8> sampler_id_5_local_t;
// sampler_id_6: 默认配置
typedef ap_fixed<24, 8> sampler_id_6_local_t;
// sampler_id_7: 默认配置
typedef ap_fixed<24, 8> sampler_id_7_local_t;
// sampler_id_8: 默认配置
typedef ap_fixed<24, 8> sampler_id_8_local_t;
// seed1: 默认配置
typedef ap_fixed<24, 8> seed1_local_t;
// seed2: 默认配置
typedef ap_fixed<24, 8> seed2_local_t;
// seed3: 默认配置
typedef ap_fixed<24, 8> seed3_local_t;
// seed4: 默认配置
typedef ap_fixed<24, 8> seed4_local_t;
// seed5: 默认配置
typedef ap_fixed<24, 8> seed5_local_t;
// seed6: 默认配置
typedef ap_fixed<24, 8> seed6_local_t;
// seed7: 默认配置
typedef ap_fixed<24, 8> seed7_local_t;
// seed8: 默认配置
typedef ap_fixed<24, 8> seed8_local_t;
// local_temp: 临时计算变量，需要足够精度
typedef ap_fixed<32, 12> local_temp_local_t;
// temp_2: 临时计算变量，需要足够精度
typedef ap_fixed<32, 12> temp_2_local_t;
// minval: 默认配置
typedef ap_fixed<24, 8> minval_local_t;
// val_1: 默认配置
typedef ap_fixed<24, 8> val_1_local_t;
// denominator: 默认配置
typedef ap_fixed<24, 8> denominator_local_t;
// temp_1: 临时计算变量，需要足够精度
typedef ap_fixed<32, 12> temp_1_local_t;
// local_temp_1: 临时计算变量，需要足够精度
typedef ap_fixed<32, 12> local_temp_1_local_t;
// local_temp_2: 临时计算变量，需要足够精度
typedef ap_fixed<32, 12> local_temp_2_local_t;
// local_temp_3: 临时计算变量，需要足够精度
typedef ap_fixed<32, 12> local_temp_3_local_t;
// local_temp_4: 临时计算变量，需要足够精度
typedef ap_fixed<32, 12> local_temp_4_local_t;
// truncated: 默认配置
typedef ap_fixed<24, 8> truncated_local_t;
// temp: 临时计算变量，需要足够精度
typedef ap_fixed<32, 12> temp_local_t;
// divisor: 默认配置
typedef ap_fixed<24, 8> divisor_local_t;
// temp_3: 临时计算变量，需要足够精度
typedef ap_fixed<32, 12> temp_3_local_t;
// step_temp_: 临时计算变量，需要足够精度
typedef ap_fixed<32, 12> step_temp__local_t;
// numerator: 默认配置
typedef ap_fixed<24, 8> numerator_local_t;
// tttt: 默认配置
typedef ap_fixed<24, 8> tttt_local_t;
// exponent: 默认配置
typedef ap_fixed<24, 8> exponent_local_t;
// tttt_pppp: 默认配置
typedef ap_fixed<24, 8> tttt_pppp_local_t;
// exponent_1: 默认配置
typedef ap_fixed<24, 8> exponent_1_local_t;
// log_pacc: 默认配置
typedef ap_fixed<24, 8> log_pacc_local_t;
// p_acc: 默认配置
typedef ap_fixed<24, 8> p_acc_local_t;
// delta_norm: 范数计算，需要正数表示
typedef ap_fixed<24, 10> delta_norm_local_t;
// alpha: 系数，通常在0-1范围
typedef ap_fixed<16, 2> alpha_local_t;
// dqam: 默认配置
typedef ap_fixed<24, 8> dqam_local_t;
// result: 默认配置
typedef ap_fixed<24, 8> result_local_t;
// sum: 默认配置
typedef ap_fixed<24, 8> sum_local_t;
// s: 默认配置
typedef ap_fixed<24, 8> s_local_t;
// di: 默认配置
typedef ap_fixed<24, 8> di_local_t;
// left_in: 默认配置
typedef ap_fixed<24, 8> left_in_local_t;
// up_in: 默认配置
typedef ap_fixed<24, 8> up_in_local_t;
// raw_bits: 默认配置
typedef ap_fixed<24, 8> raw_bits_local_t;
// int_mask: 默认配置
typedef ap_fixed<24, 8> int_mask_local_t;
// int_bits: 默认配置
typedef ap_fixed<24, 8> int_bits_local_t;
// frac_mask: 默认配置
typedef ap_fixed<24, 8> frac_mask_local_t;
// dot_product: 默认配置
typedef ap_fixed<24, 8> dot_product_local_t;
// conj: 默认配置
typedef ap_fixed<24, 8> conj_local_t;
// prod: 默认配置
typedef ap_fixed<24, 8> prod_local_t;
// proj_coeff: 默认配置
typedef ap_fixed<24, 8> proj_coeff_local_t;
// proj: 默认配置
typedef ap_fixed<24, 8> proj_local_t;
// res: 默认配置
typedef ap_fixed<24, 8> res_local_t;
// neg_sum: 默认配置
typedef ap_fixed<24, 8> neg_sum_local_t;
// avg: 默认配置
typedef ap_fixed<24, 8> avg_local_t;
// update_val: 默认配置
typedef ap_fixed<24, 8> update_val_local_t;
// ljk: 默认配置
typedef ap_fixed<24, 8> ljk_local_t;
// ljk_H: 默认配置
typedef ap_fixed<24, 8> ljk_H_local_t;
// lik: 默认配置
typedef ap_fixed<24, 8> lik_local_t;
// update_val_1: 默认配置
typedef ap_fixed<24, 8> update_val_1_local_t;
// L_jprime_j_conj: 默认配置
typedef ap_fixed<24, 8> L_jprime_j_conj_local_t;
// local_1: 默认配置
typedef ap_fixed<24, 8> local_1_local_t;
// local_2: 默认配置
typedef ap_fixed<24, 8> local_2_local_t;
// sigma2_local: 默认配置
typedef ap_fixed<24, 8> sigma2_local_local_t;
// sigma2_1: 默认配置
typedef ap_fixed<24, 8> sigma2_1_local_t;
// sigma2_2: 默认配置
typedef ap_fixed<24, 8> sigma2_2_local_t;
// sigma2_3: 默认配置
typedef ap_fixed<24, 8> sigma2_3_local_t;
// sigma2_4: 默认配置
typedef ap_fixed<24, 8> sigma2_4_local_t;
// sigma2_5: 默认配置
typedef ap_fixed<24, 8> sigma2_5_local_t;
// sigma2_6: 默认配置
typedef ap_fixed<24, 8> sigma2_6_local_t;
// sigma2_7: 默认配置
typedef ap_fixed<24, 8> sigma2_7_local_t;
// sigma2_8: 默认配置
typedef ap_fixed<24, 8> sigma2_8_local_t;
// q: 默认配置
typedef ap_fixed<24, 8> q_local_t;
// product: 默认配置
typedef ap_fixed<24, 8> product_local_t;
// r_norm_prop: 范数计算，需要正数表示
typedef ap_fixed<24, 10> r_norm_prop_local_t;
// r_norm_survivor_final: 范数计算，需要正数表示
typedef ap_fixed<24, 10> r_norm_survivor_final_local_t;
// r1: 默认配置
typedef ap_fixed<24, 8> r1_local_t;
// r2: 默认配置
typedef ap_fixed<24, 8> r2_local_t;
// r3: 默认配置
typedef ap_fixed<24, 8> r3_local_t;
// r4: 默认配置
typedef ap_fixed<24, 8> r4_local_t;
// r5: 默认配置
typedef ap_fixed<24, 8> r5_local_t;
// r6: 默认配置
typedef ap_fixed<24, 8> r6_local_t;
// r7: 默认配置
typedef ap_fixed<24, 8> r7_local_t;
// r8: 默认配置
typedef ap_fixed<24, 8> r8_local_t;
// r_norm: 范数计算，需要正数表示
typedef ap_fixed<24, 10> r_norm_local_t;
// r_norm_survivor: 范数计算，需要正数表示
typedef ap_fixed<24, 10> r_norm_survivor_local_t;
// h_real: 默认配置
typedef ap_fixed<24, 8> h_real_local_t;
// h_imag: 默认配置
typedef ap_fixed<24, 8> h_imag_local_t;
// y_r: 默认配置
typedef ap_fixed<24, 8> y_r_local_t;
// y_i: 默认配置
typedef ap_fixed<24, 8> y_i_local_t;
// v_r: 默认配置
typedef ap_fixed<24, 8> v_r_local_t;
// v_r_2: 默认配置
typedef ap_fixed<24, 8> v_r_2_local_t;
// v_r_3: 默认配置
typedef ap_fixed<24, 8> v_r_3_local_t;
// v_r_4: 默认配置
typedef ap_fixed<24, 8> v_r_4_local_t;
// v_r_5: 默认配置
typedef ap_fixed<24, 8> v_r_5_local_t;
// v_r_6: 默认配置
typedef ap_fixed<24, 8> v_r_6_local_t;
// v_r_7: 默认配置
typedef ap_fixed<24, 8> v_r_7_local_t;
// v_r_8: 默认配置
typedef ap_fixed<24, 8> v_r_8_local_t;
// v_i: 默认配置
typedef ap_fixed<24, 8> v_i_local_t;
// v_i_2: 默认配置
typedef ap_fixed<24, 8> v_i_2_local_t;
// v_i_3: 默认配置
typedef ap_fixed<24, 8> v_i_3_local_t;
// v_i_4: 默认配置
typedef ap_fixed<24, 8> v_i_4_local_t;
// v_i_5: 默认配置
typedef ap_fixed<24, 8> v_i_5_local_t;
// v_i_6: 默认配置
typedef ap_fixed<24, 8> v_i_6_local_t;
// v_i_7: 默认配置
typedef ap_fixed<24, 8> v_i_7_local_t;
// v_i_8: 默认配置
typedef ap_fixed<24, 8> v_i_8_local_t;
// step_size: 学习率/步长，小数值
typedef ap_fixed<18, 4> step_size_local_t;
// lr: 学习率/步长，小数值
typedef ap_fixed<18, 4> lr_local_t;

// ====== 复数结构体 ======
// (保持与原始头文件一致)
