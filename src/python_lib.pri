include( $${PWD}/common.pri )

#en:python dir
#cn:python的目录
DA_PYTHON_DIR = $${DA_BIN_DIR}/Python 

#en:The version of python must be consistent with the libs library in the python folder
#cn:python的版本，必须和python文件夹下libs库的一致
DA_PYTHON = python38 

#en:The following automatic loading
#cn:以下自动加载
DA_PYTHON_LIBDIR = $${DA_PYTHON_DIR}/libs/
DA_PYTHON_INCLUDE_PATH = $${DA_PYTHON_DIR}/include/

INCLUDEPATH += $${DA_PYTHON_INCLUDE_PATH}
DEPENDPATH += $${DA_PYTHON_INCLUDE_PATH}

LIBS += -L$${DA_PYTHON_LIBDIR}  -l$${DA_PYTHON}

message(DA_PYTHON_LIBDIR=$${DA_PYTHON_LIBDIR})
message(DA_PYTHON_INCLUDE_PATH=$${DA_PYTHON_INCLUDE_PATH})
