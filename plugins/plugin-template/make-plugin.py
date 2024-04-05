import json
import os
# 打开JSON文件并读取其内容
def load_template():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(script_dir,'template.json'), 'r', encoding='utf-8') as f:
        data = json.load(f)
        return data
    return {}

def make_plugin(template):
    script_dir = os.path.dirname(os.path.abspath(__file__))
    # 获取script_dir的上级目录
    parent_dir = os.path.dirname(script_dir)
    # plugin name
    basename = template["plugin-base-name"]
    # 在parent_dir中根据basename，生成一个文件夹
    new_plugin_dir = os.path.join(parent_dir,basename)
    os.mkdir(new_plugin_dir)
    # 组成template_dir
    template_dir = os.path.join(script_dir,'template')
    # 遍历template下的文件
    for filename in os.listdir(template_dir):
        # 拼接文件路径
        file_path = os.path.join(template_dir, filename)
        # 如果是文件
        if os.path.isfile(file_path):
            # 读取文件内容
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            # 替换模板中的变量
            content = content.replace('{{plugin-base-name}}', basename)
            content = content.replace('{{PLUGIN-BASE-NAME}}', basename.upper())
            content = content.replace('{{plugin-display-name}}', template['plugin-display-name'])
            content = content.replace('{{plugin-description}}', template['plugin-description'])
            content = content.replace('{{plugin-iid}}', template['plugin-iid'])
            content = content.replace('{{factory-prototypes}}', template['factory-prototypes'])
            content = content.replace('{{factory-name}}', template['factory-name'])
            content = content.replace('{{factory-description}}', template['factory-description'])
            # 在把替换后的内容重新写入到template_dir下，同时如果原来文件名带有'-basename-'则把这个替换为basename形成新的文件名
            new_file_path = os.path.join(new_plugin_dir, filename.replace('-basename-', basename))
            with open(new_file_path, 'w', encoding='utf-8') as f:
                f.write(content)
    


template = load_template()
print(template)
make_plugin(template)

