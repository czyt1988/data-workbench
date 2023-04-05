#include "DAPyModuleDatetime.h"
#include <QDebug>
#include <QObject>
namespace DA
{
class DAPyModuleDatetimePrivate
{
public:
    DA_IMPL_PUBLIC(DAPyModuleDatetime)
    DAPyModuleDatetimePrivate(DAPyModuleDatetime* p);

    //释放模块
    void del();

public:
    QString _lastErrorString;
    pybind11::object _objDatetime;
    pybind11::object _objDate;
    pybind11::object _objTime;
    pybind11::object _objTimedelta;
};
}  // namespace DA

//===================================================
// using DA namespace -- 禁止在头文件using！！
//===================================================

using namespace DA;

//===================================================
// DAPyModuleDatetimePrivate
//===================================================

DAPyModuleDatetimePrivate::DAPyModuleDatetimePrivate(DAPyModuleDatetime* p) : q_ptr(p)
{
}

void DAPyModuleDatetimePrivate::del()
{
    if (!q_ptr->isImport()) {
        return;
    }
    q_ptr->object() = pybind11::none();
}
//===================================================
// DAPyModuleDatetime
//===================================================
DAPyModuleDatetime::DAPyModuleDatetime() : DAPyModule(), d_ptr(new DAPyModuleDatetimePrivate(this))
{
    import();
    try {
        d_ptr->_objDatetime  = attr("datetime");
        d_ptr->_objDate      = attr("date");
        d_ptr->_objTime      = attr("time");
        d_ptr->_objTimedelta = attr("timedelta");
    } catch (const std::exception& e) {
        d_ptr->_lastErrorString = e.what();
    }
}

DAPyModuleDatetime::~DAPyModuleDatetime()
{
}

DAPyModuleDatetime& DAPyModuleDatetime::getInstance()
{
    static DAPyModuleDatetime s_datetime;
    return s_datetime;
}

void DAPyModuleDatetime::finalize()
{
    d_ptr->del();
}

QString DAPyModuleDatetime::getLastErrorString()
{
    return d_ptr->_lastErrorString;
}

pybind11::object DAPyModuleDatetime::dateObject()
{
    return attr("date");
}

pybind11::object DAPyModuleDatetime::dateObject(const QDate& d)
{
    pybind11::object dateobj = dateObject();
    try {
        pybind11::object init = dateobj.attr("__init__");
        init(pybind11::int_(d.year()), pybind11::int_(d.month()), pybind11::int_(d.day()));
        return dateobj;
    } catch (const std::exception& e) {
        d_ptr->_lastErrorString = e.what();
    }
    return pybind11::none();
}

pybind11::object DAPyModuleDatetime::datetimeObject()
{
    return attr("datetime");
}

pybind11::object DAPyModuleDatetime::datetimeObject(const QDateTime& d)
{
    pybind11::object obj = datetimeObject();
    try {
        pybind11::object init = obj.attr("__init__");
        init(pybind11::int_(d.date().year()),
             pybind11::int_(d.date().month()),
             pybind11::int_(d.date().day()),
             pybind11::int_(d.time().hour()),
             pybind11::int_(d.time().minute()),
             pybind11::int_(d.time().second()));
        return obj;
    } catch (const std::exception& e) {
        d_ptr->_lastErrorString = e.what();
    }
    return pybind11::none();
}

pybind11::object DAPyModuleDatetime::timeObject()
{
    return attr("time");
}

pybind11::object DAPyModuleDatetime::timeObject(const QTime& d)
{
    pybind11::object obj = timeObject();
    try {
        pybind11::object init = obj.attr("__init__");
        init(pybind11::int_(d.hour()), pybind11::int_(d.minute()), pybind11::int_(d.second()));
        return obj;
    } catch (const std::exception& e) {
        d_ptr->_lastErrorString = e.what();
    }
    return pybind11::none();
}

pybind11::object DAPyModuleDatetime::timedeltaObject()
{
    return attr("timedelta");
}

bool DAPyModuleDatetime::isInstanceTime(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->_objTime);
}

bool DAPyModuleDatetime::isInstanceDate(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->_objDate);
}

bool DAPyModuleDatetime::isInstanceDateTime(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->_objDatetime);
}

bool DAPyModuleDatetime::isInstanceTimedelta(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->_objTimedelta);
}

bool DAPyModuleDatetime::import()
{
    return DAPyModule::import("datetime");
}
