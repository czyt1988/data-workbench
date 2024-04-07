# -*- coding: utf-8 -*-

import os
from typing import List,Dict,Optional
import pandas as pd
import numpy as np


def spectrum_analysis(waveform, sampling_rate, fftsize=None):
    '''
    频谱分析

    :param waveform: 输入波形数据，可以是一维数组或列表  
    :param sampling_rate: 采样率  
    :param fftsize: FFT变换的窗口大小，默认为None，表示使用波形的长度  
    :return: 频谱分析结果，包含频率、振幅和相位三个部分  
    :rtype: pd.DataFrame
    '''
    if fftsize is None:  
        fftsize = len(waveform)  # 如果没有指定fftsize或者fftsize为None，则使用波形的长度  
  
    # 如果fftsize大于波形长度，对波形进行补零  
    if fftsize > len(waveform):  
        waveform = np.pad(waveform, (0, fftsize - len(waveform)))  
    # 如果fftsize小于波形长度，截取波形  
    elif fftsize < len(waveform):  
        waveform = waveform[:fftsize]  
  
    # 对波形执行FFT变换  
    fft_values = np.fft.rfft(waveform)  
      
    # 获取FFT结果的频率值  
    freq = np.fft.rfftfreq(fftsize, 1.0 / sampling_rate)  
      
    # 计算频谱的振幅  
    amplitudes = np.abs(fft_values) / fftsize  # 先不进行乘以2的操作  
      
    # 对非直流分量和非Nyquist分量的振幅乘以2  
    # 直流分量是第一个元素，如果fftsize是偶数，Nyquist分量是最后一个元素  
    if fftsize % 2 == 0:  # 偶数个点  
        amplitudes[1:-1] *= 2  # 忽略直流分量和Nyquist分量  
    else:  # 奇数个点，没有Nyquist分量  
        amplitudes[1:] *= 2  # 只忽略直流分量  
      
    # 计算频谱的相位  
    phases = np.angle(fft_values)  
      
    # 构造DataFrame  
    df = pd.DataFrame({  
        'Frequency': freq,  
        'Amplitude': amplitudes,  
        'Phase': phases  
    })  
      
    return df