#include "DADataEnumStringUtils.h"

// ================================== DAAbstractData::DataType ==================================
DA_ENUM_STRING_INSENSITIVE_DEFINE(DA::DAAbstractData::DataType,
                                  DA::DAAbstractData::TypeNone,
                                  { DA::DAAbstractData::TypeNone, "None" },
                                  { DA::DAAbstractData::TypeDataPackage, "Package" },
                                  { DA::DAAbstractData::TypePythonObject, "Object" },
                                  { DA::DAAbstractData::TypePythonDataFrame, "DataFrame" },
                                  { DA::DAAbstractData::TypePythonSeries, "Series" },
                                  { DA::DAAbstractData::TypeInnerData, "InnerData" });
