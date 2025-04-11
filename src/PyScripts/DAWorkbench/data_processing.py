# -*- coding: utf-8 -*-

import os
from typing import List,Dict,Optional
import pandas as pd
import numpy as np
import scipy
from DAWorkbench.logger import log_function_call  # type: ignore # 引入装饰器
from loguru import logger

@log_function_call
def spectrum_analysis(waveform, sampling_rate, fftsize=None,phases=False,
                      nextpower2=False,db=False,detrend=None):
    '''
    频谱分析,此函数主要针对一维波形进行频谱分析

    :param waveform: 输入波形数据，可以是一维数组或列表  
    :param sampling_rate: 采样率  
    :param fftsize: FFT变换的窗口大小，默认为None，表示使用波形的长度  
    :param phases: 是否计算相位，如果为True，则计算相位，否则不计算相位
    :param nextpower2: 是否将FFT变换的窗口大小取下一个2的整数次幂，默认为False，表示不取下一个2的整数次幂
                    此参数仅在fftsize为None或负数时生效
    :param db: 是否将振幅转换为分贝值，如果为True，则将振幅转换为分贝值，否则不转换
    :param detrend: 是否进行去趋势处理，'linear','constant'两个参数可选
    :return: 频谱分析结果，包含频率、振幅(或相位)两(三)个部分  
    :rtype: phases=False tuple(freq(频率):np.ndarray, amplitudes(幅值):np.ndarray)
            phases=True tuple(freq(频率):np.ndarray, amplitudes(幅值):np.ndarray ,phases(相位):np.ndarray)
    '''
    #通过loguru打印函数的所有输入的参数
    if fftsize is None or fftsize <= 1: #fftsize=1或负数是没有意义的，这里一并处理为波形长度 
        if nextpower2:  # 如果nextpower2为True，则将fftsize取下一个2的整数次幂  
            # fftsize = 2 ** int(np.ceil(np.log2(len(waveform))))
            fftsize = scipy.fft.next_fast_len(len(waveform))
        else:
            fftsize = len(waveform)  # 如果没有指定fftsize或者fftsize为None，则使用波形的长度  
    else:
        # 如果fftsize大于波形长度，对波形进行补零  
        if fftsize > len(waveform):  
            waveform = np.pad(waveform, (0, fftsize - len(waveform)))  
        # 如果fftsize小于波形长度，截取波形  
        elif fftsize < len(waveform):  
            waveform = waveform[:fftsize]

    #如果detrend不是None,则调用scipy.signal.detrend函数对波形进行去趋势处理
    if detrend is not None:  
        waveform = scipy.signal.detrend(waveform, type=detrend)

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

@log_function_call
def butterworth_filter(waveform, sampling_freq, filter_order, filter_type = 'lowpass', cutoff_freq = 0, 
                        upper_freq = 0, lower_freq = 0,  phases = False):
    '''
    巴特沃斯滤波器
    
    :param waveform:输入波形数据
    :param cutoff_freq:滤波器截止频率，标量或长度为2的序列，
        如果cutoff_freq为长度为2的序列，要保证cutoff_freq[0] < cutoff_freq[1]
    :param sampling_freq:采样频率
    :param filter_order:滤波器阶数
    :param filter_type:滤波器类型，可以选择lowpass, highpass, bandpass, bandstop
    :param phases:是否计算相位，如果为True，则计算相位，否则不计算相位，默认不计算
    :return: 滤波后的波形结果
    '''
    nyq = 0.5 * sampling_freq # 奈奎斯特采样频率
    
    # 如果滤波器类型位低通/高通滤波，则通过截止频率来计算归一化频率，传入参数为截止频率(数)
    if filter_type in ['lowpass', 'highpass']: 
        # 计算归一化截止频率
        normal_cutoff = cutoff_freq / nyq
        
        b, a = scipy.signal.butter(filter_order, normal_cutoff, btype=filter_type, analog=False) 
    
    # 如果滤波器类型位带通/带阻滤波，则通过截止频率上下限来计算归一化频率，传入参数为频率上下限(一维数组)
    elif filter_type in ['bandpass', 'bandstop']: 
        # 计算归一化截止频率上下限
        normal_lower_freq = lower_freq / nyq
        normal_upper_freq = upper_freq / nyq
        
        b, a = scipy.signal.butter(filter_order, [normal_lower_freq, normal_upper_freq], btype = filter_type, analog=False)
    
    # 计算滤波之后的数据
    if phases: # 如果phase为True，则计算为零相位差
        filtered_data = scipy.signal.filtfilt(b, a, waveform)
    elif not phases: # 如果为False,则在滤波时引入延迟
        filtered_data = scipy.signal.lfilter(b, a, waveform)
        
    return filtered_data
    
@log_function_call
def peak_analysis(waveform, sampling_rate, height = None, direction = 0, threshold = None, distance = None, prominence = None,
                  width = None, wlen = None, rel_height = 0.5, plateau_size = None):
    '''
    峰值分析
    :param waveform : 输入波形数据
    :param height : 基线高度
    :param direction : 寻峰方向
    :param threshold : 峰值与其相邻样本的最小垂直距离
    :param distance : 相邻峰值之间的最小水平距离（以样本数为单位）
    :param prominence : 指定峰值的最小突出度
    :param width : 指定峰值的最小宽度（以样本数为单位）
    :param wlen : 计算突出度和宽度时使用的窗口长度
    :param rel_height : 用于宽度计算的相对高度（0-1之间）
    :param plateau_size : 指定平顶峰值的最小尺寸
    :return : 峰值索引，峰值高度，宽度边界位置，峰值突出度，峰值宽度
    '''
    peak_data = []
    udata = waveform
    # 计算峰值相关数据
    if direction == 0 or direction == 2:
        upeak,upeak_pro = scipy.signal.find_peaks(udata, height, threshold, distance, prominence, width, wlen, rel_height, plateau_size)
        # 处理峰值数据
        for i, idx in enumerate(upeak):
            props = {}
            for key, val in upeak_pro.items():
                if isinstance(val, np.ndarray):
                    props[key] = val[i]
                else:
                    props[key] = val
            
            peak_data.append({
                'index': idx / sampling_rate,
                'value': waveform[idx],
                'properties': props
            })
            
    if direction == 1 or direction == 2:
        ddata = [2 * height - x for x in waveform]
        dpeak,dpeak_pro = scipy.signal.find_peaks(ddata, -height, threshold, distance, prominence, width, wlen, rel_height, plateau_size)
        # 处理谷值数据
        for i, idx in enumerate(dpeak):
            props = {}
            for key, val in dpeak_pro.items():
                if isinstance(val, np.ndarray):
                        props[key] = val[i]
                else:
                    props[key] = val
            
            peak_data.append({
                'index': idx / sampling_rate,
                'value': waveform[idx],
                'properties': props
            })
    # 按索引排序
    if direction == 2:
        peak_data.sort(key=lambda x: x['index'])

    return peak_data

@log_function_call
def da_spectrum_analysis(waveform, sampling_rate, args:Optional[Dict] = None):
    '''
    频谱分析,此函数主要针对一维波形进行频谱分析
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

@log_function_call
def da_butterworth_filter(waveform, sampling_freq, filter_order, args:Optional[Dict] = None):
    '''
    巴特沃斯滤波器，此函数主要针对一维波形进行滤波
    :param waveform: 输入波形数据，可以是一维数组或列表
    :param sampling_freq: 采样频率
    :return: 滤波结果，一个dataframe，包含滤波后的波形数据
    '''
    res = butterworth_filter(waveform, sampling_freq, filter_order, **args)
    return pd.DataFrame({
        'filtered_wave': res
    })
    
@log_function_call
def da_peak_analysis(waveform, sampling_rate, args:Optional[Dict] = None):
    '''
    峰值分析
    :param waveform: 输入波形数据，可以是一维数组或列表
    :param height : 基线高度
    :param direction : 寻峰方向
    :param threshold : 峰值与其相邻样本的最小垂直距离
    :param distance : 相邻峰值之间的最小水平距离（以样本数为单位）
    :param prominence : 指定峰值的最小突出度
    :param width : 指定峰值的最小宽度（以样本数为单位）
    :param wlen : 计算突出度和宽度时使用的窗口长度
    :param rel_height : 用于宽度计算的相对高度（0-1之间）
    :param plateau_size : 指定平顶峰值的最小尺寸
    :return : 峰值索引，峰值高度，宽度边界位置，峰值突出度，峰值宽度
    '''
    res = peak_analysis(waveform,sampling_rate, **args)
    peak_data = pd.DataFrame(
        [{'index': item['index'],'value': item['value']}for item in res]
        )
    properties = set()
    for item in res:
        properties.update(item['properties'].keys())
    # 为每个属性创建一个列
    for key in properties:
        peak_data[key] = [item['properties'].get(key, None) for item in res]
    return peak_data

if __name__ == '__main__':
    # 获取函数spectrum_analysis的注解
    annotations = spectrum_analysis.__annotations__
    print(f'annotations:{annotations}')
    doc = spectrum_analysis.__doc__
    print(f'annotations:{doc}')