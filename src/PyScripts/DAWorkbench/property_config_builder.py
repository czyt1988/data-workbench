# property_config_builder.py
import json
from typing import Any, Dict, List, Optional, Union, Tuple


class PropertyConfigBuilder:
    """通用属性配置构建器
    
    用于方便地构建符合DACommonPropertySettingDialog要求的JSON配置。
    支持链式调用，可以轻松构建复杂的属性配置。
    """
    
    def __init__(self, window_title: str = "参数设置"):
        """初始化配置构建器
        
        Args:
            window_title: 窗口标题
        """
        self._config = {
            "window_title": window_title,
            "properties": []
        }
        self._current_group = None
        
    def add_property(self, 
                    name: str, 
                    prop_type: str, 
                    display_name: Optional[str] = None,
                    default_value: Any = None,
                    description: Optional[str] = None,
                    **kwargs) -> 'PropertyConfigBuilder':
        """添加属性
        
        Args:
            name: 属性名称（唯一标识）
            prop_type: 属性类型，支持：string, int, double, bool, color, font, 
                      enum, file, folder, stringlist, group
            display_name: 显示名称，如未指定则使用name
            default_value: 默认值
            description: 属性描述
            **kwargs: 其他属性特定参数
            
        Returns:
            返回自身以支持链式调用
        """
        prop_config = {
            "name": name,
            "type": prop_type,
            "display_name": display_name or name
        }
        
        if default_value is not None:
            prop_config["value"] = default_value
            
        if description:
            prop_config["description"] = description
            
        # 添加特定类型的参数
        for key, value in kwargs.items():
            if value is not None:
                prop_config[key] = value
                
        # 如果是分组，需要特殊处理
        if prop_type == "group":
            prop_config["properties"] = []
            self._config["properties"].append(prop_config)
            self._current_group = prop_config
        else:
            if self._current_group:
                self._current_group["properties"].append(prop_config)
            else:
                self._config["properties"].append(prop_config)
                
        return self
    
    def add_string(self, 
                   name: str, 
                   display_name: Optional[str] = None,
                   default_value: str = "",
                   description: Optional[str] = None,
                   read_only: bool = False) -> 'PropertyConfigBuilder':
        """添加字符串属性
        
        Args:
            name: 属性名称
            display_name: 显示名称
            default_value: 默认值
            description: 属性描述
            read_only: 是否只读
            
        Returns:
            返回自身以支持链式调用
        """
        return self.add_property(
            name=name,
            prop_type="string",
            display_name=display_name,
            default_value=default_value,
            description=description,
            read_only=read_only
        )
    
    def add_int(self,
                name: str,
                display_name: Optional[str] = None,
                default_value: int = 0,
                min_value: Optional[int] = None,
                max_value: Optional[int] = None,
                description: Optional[str] = None,
                single_step: Optional[int] = None,
                read_only: bool = False) -> 'PropertyConfigBuilder':
        """添加整数属性
        
        Args:
            name: 属性名称
            display_name: 显示名称
            default_value: 默认值
            min_value: 最小值
            max_value: 最大值
            description: 属性描述
            single_step: 步进值
            read_only: 是否只读
            
        Returns:
            返回自身以支持链式调用
        """
        kwargs = {}
        if min_value is not None:
            kwargs["min"] = min_value
        if max_value is not None:
            kwargs["max"] = max_value
        if single_step is not None:
            kwargs["singleStep"] = single_step
            
        return self.add_property(
            name=name,
            prop_type="int",
            display_name=display_name,
            default_value=default_value,
            description=description,
            read_only=read_only,
            **kwargs
        )
    
    def add_double(self,
                   name: str,
                   display_name: Optional[str] = None,
                   default_value: float = 0.0,
                   min_value: Optional[float] = None,
                   max_value: Optional[float] = None,
                   decimals: int = 2,
                   description: Optional[str] = None,
                   single_step: Optional[float] = None,
                   read_only: bool = False) -> 'PropertyConfigBuilder':
        """添加浮点数属性
        
        Args:
            name: 属性名称
            display_name: 显示名称
            default_value: 默认值
            min_value: 最小值
            max_value: 最大值
            decimals: 小数位数
            description: 属性描述
            single_step: 步进值
            read_only: 是否只读
            
        Returns:
            返回自身以支持链式调用
        """
        kwargs = {
            "decimals": decimals
        }
        if min_value is not None:
            kwargs["min"] = min_value
        if max_value is not None:
            kwargs["max"] = max_value
        if single_step is not None:
            kwargs["singleStep"] = single_step
            
        return self.add_property(
            name=name,
            prop_type="double",
            display_name=display_name,
            default_value=default_value,
            description=description,
            read_only=read_only,
            **kwargs
        )
    
    def add_bool(self,
                 name: str,
                 display_name: Optional[str] = None,
                 default_value: bool = False,
                 description: Optional[str] = None) -> 'PropertyConfigBuilder':
        """添加布尔属性
        
        Args:
            name: 属性名称
            display_name: 显示名称
            default_value: 默认值
            description: 属性描述
            
        Returns:
            返回自身以支持链式调用
        """
        return self.add_property(
            name=name,
            prop_type="bool",
            display_name=display_name,
            default_value=default_value,
            description=description
        )
    
    def add_enum(self,
                 name: str,
                 display_name: Optional[str] = None,
                 default_value: str = "",
                 enum_items: List[str] = None,
                 enum_descriptions: Optional[List[str]] = None,
                 description: Optional[str] = None,
                 read_only: bool = False) -> 'PropertyConfigBuilder':
        """添加枚举属性
        
        Args:
            name: 属性名称
            display_name: 显示名称
            default_value: 默认值
            enum_items: 枚举值列表
            enum_descriptions: 枚举描述列表，长度需与enum_items相同
            description: 属性描述
            read_only: 是否只读
            
        Returns:
            返回自身以支持链式调用
            
        Raises:
            ValueError: 当enum_descriptions不为None且长度与enum_items不同时
        """
        if enum_items is None:
            enum_items = []
            
        kwargs = {
            "enum_items": enum_items
        }
        
        if enum_descriptions:
            if len(enum_descriptions) != len(enum_items):
                raise ValueError("enum_descriptions的长度必须与enum_items相同")
            kwargs["enum_descriptions"] = enum_descriptions
            
        return self.add_property(
            name=name,
            prop_type="enum",
            display_name=display_name,
            default_value=default_value,
            description=description,
            read_only=read_only,
            **kwargs
        )
    
    def add_color(self,
                  name: str,
                  display_name: Optional[str] = None,
                  default_value: str = "#000000",
                  description: Optional[str] = None) -> 'PropertyConfigBuilder':
        """添加颜色属性
        
        Args:
            name: 属性名称
            display_name: 显示名称
            default_value: 默认颜色（十六进制格式，如"#FF0000"）
            description: 属性描述
            
        Returns:
            返回自身以支持链式调用
        """
        return self.add_property(
            name=name,
            prop_type="color",
            display_name=display_name,
            default_value=default_value,
            description=description
        )
    
    def add_font(self,
                 name: str,
                 display_name: Optional[str] = None,
                 default_value: str = "Arial,12,-1,5,50,0,0,0,0,0",
                 description: Optional[str] = None) -> 'PropertyConfigBuilder':
        """添加字体属性
        
        Args:
            name: 属性名称
            display_name: 显示名称
            default_value: 默认字体字符串
            description: 属性描述
            
        Returns:
            返回自身以支持链式调用
        """
        return self.add_property(
            name=name,
            prop_type="font",
            display_name=display_name,
            default_value=default_value,
            description=description
        )
    
    def add_file(self,
                 name: str,
                 display_name: Optional[str] = None,
                 default_value: str = "",
                 file_filter: str = "所有文件 (*.*)",
                 description: Optional[str] = None) -> 'PropertyConfigBuilder':
        """添加文件选择属性
        
        Args:
            name: 属性名称
            display_name: 显示名称
            default_value: 默认文件路径
            file_filter: 文件过滤器（如"文本文件 (*.txt);;所有文件 (*.*)"）
            description: 属性描述
            
        Returns:
            返回自身以支持链式调用
        """
        return self.add_property(
            name=name,
            prop_type="file",
            display_name=display_name,
            default_value=default_value,
            description=description,
            filter=file_filter
        )
    
    def add_folder(self,
                   name: str,
                   display_name: Optional[str] = None,
                   default_value: str = "",
                   description: Optional[str] = None) -> 'PropertyConfigBuilder':
        """添加文件夹选择属性
        
        Args:
            name: 属性名称
            display_name: 显示名称
            default_value: 默认文件夹路径
            description: 属性描述
            
        Returns:
            返回自身以支持链式调用
        """
        return self.add_property(
            name=name,
            prop_type="folder",
            display_name=display_name,
            default_value=default_value,
            description=description
        )
    
    def add_stringlist(self,
                       name: str,
                       display_name: Optional[str] = None,
                       default_value: List[str] = None,
                       description: Optional[str] = None) -> 'PropertyConfigBuilder':
        """添加字符串列表属性
        
        Args:
            name: 属性名称
            display_name: 显示名称
            default_value: 默认字符串列表
            description: 属性描述
            
        Returns:
            返回自身以支持链式调用
        """
        if default_value is None:
            default_value = []
            
        return self.add_property(
            name=name,
            prop_type="stringlist",
            display_name=display_name,
            default_value=default_value,
            description=description
        )
    
    def begin_group(self,
                    name: str,
                    display_name: Optional[str] = None,
                    description: Optional[str] = None) -> 'PropertyConfigBuilder':
        """开始一个分组
        
        Args:
            name: 分组名称
            display_name: 分组显示名称
            description: 分组描述
            
        Returns:
            返回自身以支持链式调用
        """
        return self.add_property(
            name=name,
            prop_type="group",
            display_name=display_name,
            description=description
        )
    
    def end_group(self) -> 'PropertyConfigBuilder':
        """结束当前分组
        
        Returns:
            返回自身以支持链式调用
        """
        self._current_group = None
        return self
    
    def to_json(self, indent: int = 4, ensure_ascii: bool = False) -> str:
        """转换为JSON字符串
        
        Args:
            indent: 缩进空格数
            ensure_ascii: 是否确保ASCII编码
            
        Returns:
            JSON格式的配置字符串
        """
        return json.dumps(self._config, indent=indent, ensure_ascii=ensure_ascii)
    
    def to_dict(self) -> Dict:
        """转换为字典
        
        Returns:
            配置字典
        """
        return self._config.copy()
    
    @classmethod
    def from_function_signature(cls, 
                               function, 
                               window_title: str = "参数设置",
                               param_defaults: Dict[str, Any] = None) -> 'PropertyConfigBuilder':
        """从函数签名创建配置构建器（高级功能）
        
        Args:
            function: 函数对象，需要从中获取参数信息
            window_title: 窗口标题
            param_defaults: 参数默认值覆盖
            
        Returns:
            配置构建器实例
            
        Note:
            这是一个高级功能，可以根据函数的参数自动生成配置。
            需要函数有类型注解和/或默认值。
        """
        import inspect
        
        builder = cls(window_title)
        sig = inspect.signature(function)
        
        if param_defaults is None:
            param_defaults = {}
            
        for param_name, param in sig.parameters.items():
            # 跳过self参数
            if param_name == 'self':
                continue
                
            # 获取默认值
            default_value = param_defaults.get(param_name, 
                                             param.default if param.default != inspect.Parameter.empty else None)
            
            # 根据类型注解推断属性类型
            param_type = param.annotation
            
            if param_type == str or (hasattr(param_type, '__origin__') and param_type.__origin__ == str):
                builder.add_string(param_name, default_value=default_value or "")
            elif param_type == int or (hasattr(param_type, '__origin__') and param_type.__origin__ == int):
                builder.add_int(param_name, default_value=default_value or 0)
            elif param_type == float or (hasattr(param_type, '__origin__') and param_type.__origin__ == float):
                builder.add_double(param_name, default_value=default_value or 0.0)
            elif param_type == bool or (hasattr(param_type, '__origin__') and param_type.__origin__ == bool):
                builder.add_bool(param_name, default_value=default_value or False)
            else:
                # 默认为字符串类型
                builder.add_string(param_name, default_value=str(default_value) if default_value else "")
                
        return builder