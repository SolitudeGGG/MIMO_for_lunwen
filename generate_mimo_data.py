#!/usr/bin/env python3
"""
MIMO数据生成脚本
使用commpy库生成MIMO系统测试数据，包括H矩阵、y向量和参考比特流
支持4QAM/16QAM/64QAM调制，SNR范围5-25dB
"""

import numpy as np
import os
import sys

try:
    from commpy.modulation import QAMModem
except ImportError:
    print("错误: 未找到commpy库，请先安装: pip install scikit-commpy")
    sys.exit(1)

def generate_complex_gaussian_noise(size, variance=1.0):
    """生成复高斯噪声"""
    real_part = np.random.randn(size) * np.sqrt(variance / 2)
    imag_part = np.random.randn(size) * np.sqrt(variance / 2)
    return real_part + 1j * imag_part

def generate_channel_matrix(Nt, Nr):
    """生成瑞利衰落信道矩阵 (Rayleigh fading channel)"""
    H_real = np.random.randn(Nr, Nt) / np.sqrt(2)
    H_imag = np.random.randn(Nr, Nt) / np.sqrt(2)
    H = H_real + 1j * H_imag
    return H

def generate_mimo_data(Nt, Nr, modulation_order, snr_db, num_samples=10, output_dir="mimo_data"):
    """
    生成MIMO测试数据
    
    参数:
        Nt: 发送天线数
        Nr: 接收天线数
        modulation_order: 调制阶数 (4, 16, 64)
        snr_db: 信噪比 (dB)
        num_samples: 生成样本数
        output_dir: 输出目录
    """
    assert Nt == Nr, "当前要求发送天线和接收天线数量相同"
    
    # 创建输出目录
    os.makedirs(output_dir, exist_ok=True)
    
    # 根据调制阶数选择调制方式
    if modulation_order == 4:
        modem = QAMModem(4)  # 4-QAM (QPSK)
        bits_per_symbol = 2
        mod_name = "4QAM"
    elif modulation_order == 16:
        modem = QAMModem(16)  # 16-QAM
        bits_per_symbol = 4
        mod_name = "16QAM"
    elif modulation_order == 64:
        modem = QAMModem(64)  # 64-QAM
        bits_per_symbol = 6
        mod_name = "64QAM"
    else:
        raise ValueError(f"不支持的调制阶数: {modulation_order}")
    
    # 准备文件名
    config_str = f"{Nt}x{Nr}_{mod_name}_SNR{int(snr_db)}"
    H_file = os.path.join(output_dir, f"H_{config_str}.txt")
    y_file = os.path.join(output_dir, f"y_{config_str}.txt")
    bits_file = os.path.join(output_dir, f"bits_{config_str}.txt")
    
    # 打开文件
    f_H = open(H_file, 'w')
    f_y = open(y_file, 'w')
    f_bits = open(bits_file, 'w')
    
    # 计算噪声功率
    signal_power = Nt / Nr  # 平均每个接收天线的信号功率
    snr_linear = 10 ** (snr_db / 10)
    noise_power = signal_power / snr_linear
    
    print(f"生成配置: {config_str}")
    print(f"  调制方式: {mod_name}")
    print(f"  天线配置: {Nt}x{Nr}")
    print(f"  SNR: {snr_db} dB")
    print(f"  噪声功率: {noise_power:.6f}")
    
    for sample_idx in range(num_samples):
        # 生成随机比特流
        num_bits = Nt * bits_per_symbol
        bits = np.random.randint(0, 2, num_bits)
        
        # 调制
        symbols = modem.modulate(bits)
        
        # 确保符号数量正确
        if len(symbols) < Nt:
            # 如果符号不足，填充零
            symbols = np.concatenate([symbols, np.zeros(Nt - len(symbols))])
        elif len(symbols) > Nt:
            # 如果符号过多，截断
            symbols = symbols[:Nt]
        
        x = symbols.reshape(-1, 1)  # Nt x 1
        
        # 生成信道矩阵
        H = generate_channel_matrix(Nt, Nr)
        
        # 生成噪声
        noise = generate_complex_gaussian_noise(Nr, noise_power)
        noise = noise.reshape(-1, 1)
        
        # 计算接收信号 y = Hx + noise
        y = H @ x + noise
        
        # 写入H矩阵 (按行优先，即C/C++数组顺序)
        for i in range(Nr):
            for j in range(Nt):
                f_H.write(f"{H[i, j].real:.6f} {H[i, j].imag:.6f}\n")
        
        # 写入y向量
        for i in range(Nr):
            f_y.write(f"{y[i, 0].real:.6f} {y[i, 0].imag:.6f}\n")
        
        # 写入比特
        for bit in bits:
            f_bits.write(f"{bit}\n")
    
    f_H.close()
    f_y.close()
    f_bits.close()
    
    print(f"  生成了 {num_samples} 个样本")
    print(f"  输出文件:")
    print(f"    - {H_file}")
    print(f"    - {y_file}")
    print(f"    - {bits_file}")
    print()

def generate_gaussian_noise_file(output_file, num_samples, samplers=4):
    """
    生成高斯噪声文件供HLS使用
    每个采样器需要独立的噪声序列
    """
    print(f"生成高斯噪声文件: {output_file}")
    
    with open(output_file, 'w') as f:
        for _ in range(num_samples * samplers):
            real = np.random.randn()
            imag = np.random.randn()
            f.write(f"Real: {real:.6f}, Imaginary: {imag:.6f}\n")
    
    print(f"  生成了 {num_samples * samplers} 个噪声样本")

def main():
    """主函数：生成所有配置的MIMO数据"""
    
    # 固定天线配置
    Nt = Nr = 8
    
    # 调制阶数列表
    modulation_orders = [4, 16, 64]
    
    # SNR范围 (5dB 到 25dB，步长5dB)
    snr_range = range(5, 30, 5)  # [5, 10, 15, 20, 25]
    
    # 每个配置生成的样本数
    num_samples = 10
    
    # 输出目录
    output_dir = "mimo_data"
    
    print("=" * 60)
    print("MIMO测试数据生成工具")
    print("=" * 60)
    print()
    
    # 生成所有配置的数据
    for mod_order in modulation_orders:
        for snr_db in snr_range:
            generate_mimo_data(Nt, Nr, mod_order, snr_db, num_samples, output_dir)
    
    # 生成高斯噪声文件
    noise_dir = os.path.join(output_dir, "gaussian_noise")
    os.makedirs(noise_dir, exist_ok=True)
    
    # 为每个采样器生成独立的噪声文件
    num_noise_samples = Nt * 10 * 100  # 足够多的噪声样本
    for sampler_id in range(1, 5):  # 4个采样器
        noise_file = os.path.join(noise_dir, f"gaussian_random_values_plus_{sampler_id}.txt")
        generate_gaussian_noise_file(noise_file, num_noise_samples // 4, samplers=1)
    
    print("=" * 60)
    print("数据生成完成！")
    print(f"所有文件保存在: {output_dir}/")
    print("=" * 60)

if __name__ == "__main__":
    main()
