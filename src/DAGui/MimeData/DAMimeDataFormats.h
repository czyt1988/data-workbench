#ifndef DAMIMEDATAFORMATS_H
#define DAMIMEDATAFORMATS_H

/**
 * @file 这里定义了DAMimeData的所以格式
 *
 * 格式的命名方式为:da-workbench/{类名}
 */

#ifndef DAMIMEDATA_FORMAT_DADATA
#define DAMIMEDATA_FORMAT_DADATA "da-workbench/DAData"
#endif

/**
 * @brief DAData中DataFrame的Series，携带DAData和Series的名字
 */
#ifndef DAMIMEDATA_FORMAT_DADATA_DATAFRAME_SERIES
#define DAMIMEDATA_FORMAT_DADATA_DATAFRAME_SERIES "da-workbench/DAData.DataFrame.Series"
#endif

/**
 * @brief 针对选中多个数据的series的情况
 */
#ifndef DAMIMEDATA_FORMAT_MULT_DADATAS_SERIES
#define DAMIMEDATA_FORMAT_MULT_DADATAS_SERIES "da-workbench/mult-DADatas.Series"
#endif
#endif  // DAMIMEDATAFORMATS_H
