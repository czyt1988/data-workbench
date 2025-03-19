# 此脚本是用来进行测试
import os
import DAWorkbench

# 获取当前工作目录
current_working_directory = os.getcwd()
print("当前工作目录:", current_working_directory)
# 获取当前脚本的绝对路径
script_path = os.path.abspath(__file__)
# 获取当前脚本所在的目录
script_dir = os.path.dirname(script_path)
print("当前脚本所在的目录:", script_dir)
def tst_dataframe():
	DAWorkbench.dataframe.__tst_fill_na()
	
def tst_io():
    # 拼接成绝对路径
    file_path = os.path.join(script_dir, '测试数据集-ANSI.csv')
    DAWorkbench.io.da_read(file_path)
	
if __name__ == '__main__':
    tst_dataframe()
    tst_io()