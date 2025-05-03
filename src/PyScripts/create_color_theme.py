# 此脚本是用来生成颜色主题
# 需要提前安装：
# pip install met_brewer
import met_brewer
cppcode_enum = '''
enum ColorThemeStyle
{
'''
cppcode_switch_create = '''
DAIndexedVector< QColor > DAColorTheme::createColorList(const ColorThemeStyle& th)
{
	switch (th) {
'''

cppcode_enumtostring = '''
QString enumToString(DAColorTheme::ColorThemeStyle th)
{
	switch (th) {
'''

cppcode_stringtoenum = '''
DAColorTheme::ColorThemeStyle stringToEnum(const QString& str, DAColorTheme::ColorThemeStyle defaultEnum)
{
	if (0 == s.compare("UserDefine", Qt::CaseInsensitive)) {
		return DAColorTheme::Style_UserDefine;
	}
'''
# 把所有配色方案打印出来
for palette_name, palette_dict in met_brewer.COLORBLIND_PALETTES.items():
    enumitem = f'Style_{palette_name}'
    orderpalette = zip(palette_dict['colors'],palette_dict['order'])
    orderpalette = sorted(orderpalette,key=lambda x:x[1])
    orderpalette = [v[0] for v in orderpalette]
    print(orderpalette)
    cppcode_enum += (enumitem + ',\n')
    qcolorlist = ''
    for c in orderpalette:
        qcolorlist += f'QColor(\"{c}\"),'
    qcolorlist = qcolorlist[:-1]
    cppcode_switch_create += f'case DAColorTheme::{enumitem}:\n\treturn DAIndexedVector<QColor>({{{qcolorlist}}});\n'
    cppcode_enumtostring += f'case DAColorTheme::{enumitem}:\n\treturn \"{palette_name}\";\n'
    cppcode_stringtoenum += f'else if (0 == s.compare(\"{palette_name}\", Qt::CaseInsensitive)) {{\n\treturn DAColorTheme::{enumitem};\n}}\n'

cppcode_enum+='Style_UserDefine = 2000 ///< 用户自定义\n};'
cppcode_switch_create += '''default:
        break;
    }
    return DAIndexedVector< QColor >();
}
'''
cppcode_enumtostring += 'default:\n\tbreak;\n}\nreturn \"UserDefine\";\n}'
cppcode_stringtoenum += '\nreturn DAColorTheme::Style_UserDefine;\n}'
print(cppcode_enum)
print('----------')
print(cppcode_switch_create)
print('----------')
print(cppcode_enumtostring)
print('----------')
print(cppcode_stringtoenum)