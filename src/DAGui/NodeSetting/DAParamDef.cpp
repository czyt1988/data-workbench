#include "DAParamDef.h"

namespace DA
{

const QString DAParamDef::PropertyName_Enum     = QStringLiteral("enum");
const QString DAParamDef::PropertyName_Min      = QStringLiteral("min");
const QString DAParamDef::PropertyName_Max      = QStringLiteral("max");
const QString DAParamDef::PropertyName_Step     = QStringLiteral("step");
const QString DAParamDef::PropertyName_Decimals = QStringLiteral("decimals");
const QString DAParamDef::PropertyName_Filter   = QStringLiteral("filter");

DAParamDef::DAParamDef() : propertyId(0)
{
}

bool DAParamDef::hasProperty(const QString& propName) const
{
    return propertys.contains(propName);
}

QStringList DAParamDef::getEnumStringListProperty() const
{
    QStringList res;
    if (hasProperty(PropertyName_Enum)) {
        QVariant val = propertys.value(PropertyName_Enum);
        if (val.canConvert< QStringList >()) {
            res = val.value< QStringList >();
        } else if (val.canConvert< QList< QPair< QString, int > > >()) {
            QList< QPair< QString, int > > pairList = val.value< QList< QPair< QString, int > > >();
            for (const auto& pair : pairList) {
                res.append(pair.first);
            }
        }
    }
    return res;
}

QList< QPair< QString, int > > DAParamDef::getEnumListProperty() const
{
    QList< QPair< QString, int > > res;
    if (hasProperty(PropertyName_Enum)) {
        QVariant val = propertys.value(PropertyName_Enum);
        if (val.canConvert< QList< QPair< QString, int > > >()) {
            res = val.value< QList< QPair< QString, int > > >();
        } else if (val.canConvert< QStringList >()) {
            QStringList strList = val.value< QStringList >();
            for (int i = 0; i < strList.size(); ++i) {
                res.append({ strList[ i ], i });
            }
        }
    }
    return res;
}

bool DAParamDef::hasEnumProperty() const
{
    return hasProperty(PropertyName_Enum);
}

int DAParamDef::getMinProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< int >(propertys, PropertyName_Min, -1, isSuccess);
}

double DAParamDef::getMinFProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< double >(propertys, PropertyName_Min, -1.0, isSuccess);
}

int DAParamDef::getMaxProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< int >(propertys, PropertyName_Max, -1, isSuccess);
}

double DAParamDef::getMaxFProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< double >(propertys, PropertyName_Max, -1.0, isSuccess);
}

int DAParamDef::getStepProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< int >(propertys, PropertyName_Step, 1, isSuccess);
}

double DAParamDef::getStepFProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< double >(propertys, PropertyName_Step, 1.0, isSuccess);
}

int DAParamDef::getDecimalsProperty(bool* isSuccess) const
{
    return Detail::getNumericProperty< int >(propertys, PropertyName_Decimals, 2, isSuccess);
}

QString DAParamDef::getFilterProperty() const
{
    return Detail::getNumericProperty< QString >(propertys, PropertyName_Filter, QString(), nullptr);
}

bool DAParamDef::hasDefaultValue() const
{
    return defaultValue.isValid();
}

QString DAParamDef::defaultValueToString() const
{
    return defaultValue.toString();
}

int DAParamDef::defaultValueToInt(bool* isSuccess) const
{
    return defaultValue.toInt(isSuccess);
}

double DAParamDef::defaultValueToDouble(bool* isSuccess) const
{
    return defaultValue.toDouble(isSuccess);
}

bool DAParamDef::defaultValueToBool() const
{
    return defaultValue.toBool();
}

}  // namespace DA