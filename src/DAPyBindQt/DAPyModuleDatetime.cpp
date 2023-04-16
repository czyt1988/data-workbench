#include "DAPyModuleDatetime.h"
#include <QDebug>
#include <QObject>
namespace DA
{
class DAPyModuleDatetime::PrivateData
{
public:
    DA_DECLARE_PUBLIC(DAPyModuleDatetime)
    PrivateData(DAPyModuleDatetime* p);

    //释放模块
    void del();

public:
    QString mLastErrorString;
    pybind11::object mObjDatetime;
    pybind11::object mObjDate;
    pybind11::object mObjTime;
    pybind11::object mObjTimedelta;
};

//===================================================
// DAPyModuleDatetimePrivate
//===================================================

DAPyModuleDatetime::PrivateData::PrivateData(DAPyModuleDatetime* p) : q_ptr(p)
{
}

void DAPyModuleDatetime::PrivateData::del()
{
    if (!q_ptr->isImport()) {
        return;
    }
    q_ptr->object() = pybind11::none();
}
//===================================================
// DAPyModuleDatetime
//===================================================
DAPyModuleDatetime::DAPyModuleDatetime() : DAPyModule(), d_ptr(new DAPyModuleDatetime::PrivateData(this))
{
    import();
    try {
        d_ptr->mObjDatetime  = attr("datetime");
        d_ptr->mObjDate      = attr("date");
        d_ptr->mObjTime      = attr("time");
        d_ptr->mObjTimedelta = attr("timedelta");
    } catch (const std::exception& e) {
        d_ptr->mLastErrorString = e.what();
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
    return d_ptr->mLastErrorString;
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
        d_ptr->mLastErrorString = e.what();
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
        d_ptr->mLastErrorString = e.what();
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
        d_ptr->mLastErrorString = e.what();
    }
    return pybind11::none();
}

pybind11::object DAPyModuleDatetime::timedeltaObject()
{
    return attr("timedelta");
}

bool DAPyModuleDatetime::isInstanceTime(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->mObjTime);
}

bool DAPyModuleDatetime::isInstanceDate(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->mObjDate);
}

bool DAPyModuleDatetime::isInstanceDateTime(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->mObjDatetime);
}

bool DAPyModuleDatetime::isInstanceTimedelta(const pybind11::object& obj) const
{
    return pybind11::isinstance(obj, d_ptr->mObjTimedelta);
}

bool DAPyModuleDatetime::import()
{
    return DAPyModule::import("datetime");
}
}  // namespace DA
