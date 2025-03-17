# 此脚本是用来生成颜色主题
# 需要提前安装：
# pip install met_brewer
import DAWorkbench

def tst_dataframe():
	DAWorkbench.dataframe.__tst_fill_na()
	
def tst_io():
	DAWorkbench.io.da_read('./测试数据集-ANSI.csv')
	
if __name__ == '__main__':
    tst_dataframe()
    tst_io()