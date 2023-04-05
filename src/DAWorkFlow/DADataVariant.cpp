#include "DADataVariant.h"

DADataVariant::DADataVariant() {}

DADataVariant::DADataVariant(const QVariant &v, const QString &n)
{
    set(v, n);
}

QVariant DADataVariant::getValue() const
{
    return (m_variant);
}

void DADataVariant::setValue(const QVariant &v)
{
    m_variant = v;
}

QVariant &DADataVariant::value()
{
    return (m_variant);
}

const QVariant &DADataVariant::value() const
{
    return (m_variant);
}

QString DADataVariant::getName() const
{
    return (m_name);
}

void DADataVariant::setName(const QString &n)
{
    m_name = n;
}

QString &DADataVariant::name()
{
    return (m_name);
}

const QString &DADataVariant::name() const
{
    return (m_name);
}

void DADataVariant::set(const QVariant &v, const QString &n)
{
    setValue(v);
    setName(n);
}

bool operator==(const DADataVariant &a, const DADataVariant &b)
{
    return ((a.value() == b.value()) && (a.name() == b.name()));
}

bool operator<(const DADataVariant &a, const DADataVariant &b)
{
    return (a.value() < b.value());
}

QDebug operator<<(QDebug dbg, const DADataVariant &a)
{
    dbg.nospace() << "(" << a.name() << ")" << a.value();
    return (dbg.space());
}
