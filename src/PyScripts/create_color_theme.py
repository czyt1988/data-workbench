# 此脚本是用来生成颜色主题
# 需要提前安装：
# pip install met_brewer
import met_brewer
cppcode_enum = ''
cppcode_switch_create = ''
# 把所有配色方案打印出来
for palette_name, palette_dict in met_brewer.COLORBLIND_PALETTES.items():
    enumitem = f'ColorTheme_{palette_name}'
    orderpalette = zip(palette_dict['colors'],palette_dict['order'])
    orderpalette = sorted(orderpalette,key=lambda x:x[1])
    orderpalette = [v[0] for v in orderpalette]
    print(orderpalette)
    cppcode_enum += (enumitem + ',\n')
    qcolorlist = ''
    for c in orderpalette:
        qcolorlist += f'QColor(\"{c}\"),'
    qcolorlist = qcolorlist[:-1]
    cppcode_switch_create += f'case DAColorTheme::{enumitem}:\n\treturn DAColorTheme({{{qcolorlist}}});\n'

print(cppcode_enum)
print('----------')
print(cppcode_switch_create)
