#ifndef DAFORMSCHEMAIO_H
#define DAFORMSCHEMAIO_H
#include "DAUtilsAPI.h"
#include "DAFormSpec.h"
#include <QJsonObject>

/**
 * @file 此文件声明了表单规格的 JSON 解析与序列化接口
 */
namespace DA
{
/**
 * @brief 表单规格 JSON 解析与序列化工具类
 *
 * 提供静态方法在 DAFormSpec 与 QJsonObject 之间双向转换。
 *
 * 解析遵循 v2 schema：仅识别 v2 的键，对未知键保持容错（跳过），
 * 但对结构性错误（如 items 非数组、字段缺少 name）会写入错误信息并返回 false。
 *
 * 序列化为解析的逆过程，固定输出 version: 2，仅在 options 非空时输出 options，
 * 仅输出 attributes 中存在的键，并省略空的 visible_when/enabled_when/required_when。
 */
class DAUTILS_API DAFormSchemaIO
{
public:
    // 从 JSON 对象解析表单规格，失败时写入 errorMessage 并返回 false
    static bool fromJsonObject(const QJsonObject& obj, DAFormSpec& spec, QString* errorMessage = nullptr);
    // 将表单规格序列化为 JSON 对象，固定输出 version: 2
    static QJsonObject toJsonObject(const DAFormSpec& spec);
};
}  // namespace DA
#endif  // DAFORMSCHEMAIO_H
