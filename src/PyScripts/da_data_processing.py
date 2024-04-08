# -*- coding: utf-8 -*-

import os
from typing import List,Dict,Optional
import pandas as pd
import numpy as np


def spectrum_analysis(waveform, sampling_rate, fftsize=None,phases=False,nextpower2=False,db=False,detrend=False):
    '''
    频谱分析

    :param waveform: 输入波形数据，可以是一维数组或列表  
    :param sampling_rate: 采样率  
    :param fftsize: FFT变换的窗口大小，默认为None，表示使用波形的长度  
    :param phases: 是否计算相位，如果为True，则计算相位，否则不计算相位
    :param nextpower2: 是否将FFT变换的窗口大小取下一个2的整数次幂，默认为False，表示不取下一个2的整数次幂
                    此参数仅在fftsize为None或负数时生效
    :param db: 是否将振幅转换为分贝值，如果为True，则将振幅转换为分贝值，否则不转换
    :param detrend: 是否进行去趋势处理，如果为True，则进行去趋势处理，否则不处理
    :return: 频谱分析结果，包含频率、振幅(或相位)两(三)个部分  
    :rtype: pd.DataFrame
    '''
    if fftsize is None or fftsize <= 1: #fftsize=1或负数是没有意义的，这里一并处理为波形长度 
        if nextpower2:  # 如果nextpower2为True，则将fftsize取下一个2的整数次幂  
            fftsize = 2 ** int(np.ceil(np.log2(len(waveform))))
        else:
            fftsize = len(waveform)  # 如果没有指定fftsize或者fftsize为None，则使用波形的长度  
    else:
        # 如果fftsize大于波形长度，对波形进行补零  
        if fftsize > len(waveform):  
            waveform = np.pad(waveform, (0, fftsize - len(waveform)))  
        # 如果fftsize小于波形长度，截取波形  
        elif fftsize < len(waveform):  
            waveform = waveform[:fftsize]

    if detrend:  # 如果需要去趋势处理,去除直流分量（即信号的平均值）  
        waveform = waveform - np.mean(waveform)

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
    
    if db:  # 如果需要将振幅转换为分贝值  
        # 为了避免对0取对数，添加一个非常小的值epsilon
        amplitudes[amplitudes == 0] = np.finfo(float).eps
        amplitudes = 20 * np.log10(amplitudes)  # 将振幅转换为分贝值  
        # amplitudes[amplitudes < -120] = -120  # 将小于-120的振幅设为-120

    # 计算频谱的相位  
    if phases:  
        phases = np.unwrap(np.angle(fft_values))
        return freq,amplitudes,phases
    return freq,amplitudes


def da_spectrum_analysis(waveform, sampling_rate, args:Optional[Dict] = None):
    '''
    频谱分析
    :param waveform: 输入波形数据，可以是一维数组或列表
    :param sampling_rate: 采样率
    :param fftsize: FFT变换的窗口大小，默认为None，表示使用波形的长度
    :param phases: 是否计算相位，如果为True，则计算相位，否则不计算相位
    :return: 频谱分析结果，一个dataframe,根据参数，包含频率、振幅，或者相位
    '''
    res = spectrum_analysis(waveform,sampling_rate,**args)
    if 3 == len(res):
        return pd.DataFrame({  
                'freq': res[0],  
                'amplitudes': res[1],  
                'phase': res[2],   
            })  
    else:
        return pd.DataFrame({  
                'freq': res[0],  
                'amplitudes': res[1],   
            })  
    