﻿#include "DAMessageHandler.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QList>
#include <QDateTime>
#include "DAMessageQueueProxy.h"
#include "da_concurrent_queue.hpp"
#include <atomic>
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

#ifndef globalMessageHandleValues
#define globalMessageHandleValues _DAMessageHandlerGlobalValues::getInstance()
#endif
namespace DA
{
/**
 * @brief 此单例管理DAMessageHandler的全局变量
 */
class _DAMessageHandlerGlobalValues
{
    _DAMessageHandlerGlobalValues();

public:
    /**
     * @brief 消息处理类型
     */
    enum class MsgHandleType
    {
        UnknowHandleType,
        HandleMsgStdout,     ///< 消息输出到stdout
        HandleMsgRotateFile  ///< 消息写入文件，文件可旋转
    };

public:
    //获取单例
    static _DAMessageHandlerGlobalValues& getInstance();
    // spdlog日志
    void setLogger(const std::shared_ptr< spdlog::logger >& logger);
    spdlog::logger* logger();
    //判断是否执行message handle，通过这个可以临时跳过一些捕获
    void setEnableMessageHandle(bool on);
    bool enableMessageHandle() const;
    //判断是否使用spdlog
    void setEnableSpdLog(bool on);
    bool enableSpdLog() const;
    //获取全局队列的引用
    DAMessageQueueProxy& msgQueue();
    //设置消息的类型
    void setMsgHandleType(MsgHandleType t);
    MsgHandleType getMsgHandleType() const;

    //设置记录进入全局消息队列的消息等级，默认为QtWarningMsg
    QtMsgType getMsgQueueRecordMsgType() const;
    void setMsgQueueRecordMsgType(QtMsgType t);
    //获取patter
    const char* getPatternChar() const;
    void setPattern(const QString& p);

private:
    QtMsgType _recordMsgType;
    /**
     * @brief 全局的spdlog日志
     */
    std::shared_ptr< spdlog::logger > _daLogger;
    /**
     * @brief m_enableMsgHandleSpdlog
     */
    std::atomic_bool _enableSpdlog;
    /**
     * @brief 消息缓存
     */
    DAMessageQueueProxy _msgQueue;
    /**
     * @brief 消息处理的类型
     */
    MsgHandleType _type;

    /**
     * @brief 记录格式
     */
    std::string _pattern;

    /**
     * @brief 允许消息捕获
     */
    std::atomic_bool _enableMsgHandle { true };
};

_DAMessageHandlerGlobalValues::_DAMessageHandlerGlobalValues()
    : _recordMsgType(QtWarningMsg), _type(MsgHandleType::UnknowHandleType), _pattern("[{1}]({2}){5}")
{
    _enableSpdlog.store(true);
    _enableMsgHandle.store(true);
}

_DAMessageHandlerGlobalValues& _DAMessageHandlerGlobalValues::getInstance()
{
    static _DAMessageHandlerGlobalValues s_msg_handle_values;
    return s_msg_handle_values;
}

void _DAMessageHandlerGlobalValues::setLogger(const std::shared_ptr< spdlog::logger >& logger)
{
    this->_daLogger = logger;
}

spdlog::logger* _DAMessageHandlerGlobalValues::logger()
{
    return this->_daLogger.get();
}

void _DAMessageHandlerGlobalValues::setEnableMessageHandle(bool on)
{
    return this->_enableMsgHandle.store(on);
}

bool _DAMessageHandlerGlobalValues::enableMessageHandle() const
{
    return this->_enableMsgHandle.load();
}

void _DAMessageHandlerGlobalValues::setEnableSpdLog(bool on)
{
    _enableSpdlog.store(on);
}

bool _DAMessageHandlerGlobalValues::enableSpdLog() const
{
    return _enableSpdlog.load();
}

DAMessageQueueProxy& _DAMessageHandlerGlobalValues::msgQueue()
{
    return _msgQueue;
}

void _DAMessageHandlerGlobalValues::setMsgHandleType(_DAMessageHandlerGlobalValues::MsgHandleType t)
{
    _type = t;
}

_DAMessageHandlerGlobalValues::MsgHandleType _DAMessageHandlerGlobalValues::getMsgHandleType() const
{
    return _type;
}

QtMsgType _DAMessageHandlerGlobalValues::getMsgQueueRecordMsgType() const
{
    return _recordMsgType;
}

void _DAMessageHandlerGlobalValues::setMsgQueueRecordMsgType(QtMsgType t)
{
    _recordMsgType = t;
}

/**
 * @brief 初始化旋转日志
 * @param filename
 * @param maxfile_size
 * @param maxfile_counts
 * @param flush_every_sec
 * @param output_stdout
 * @param async_logger
 */
void _initializeRotatingSpdlog(const std::string& filename,
                               int maxfile_size    = 1048576 * 10,
                               int maxfile_counts  = 5,
                               int flush_every_sec = 15,
                               bool output_stdout  = true,
                               bool async_logger   = true);
/**
 * @brief 消息回调
 * @param type
 * @param context
 * @param msg
 */
void _daMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

/**
 * @brief 初始化控制台日志
 * @param 刷新日志
 * @param async_logger
 */
void _initializeConsolSpdlog(int flush_every_sec, bool async_logger)
{
    std::vector< spdlog::sink_ptr > sinks;
    auto stdout_sink = std::make_shared< spdlog::sinks::stdout_color_sink_mt >();
    sinks.emplace_back(stdout_sink);

    std::shared_ptr< spdlog::logger > logger;
    if (async_logger) {
        //初始化异步线程的参数
        spdlog::init_thread_pool(10240, 1);
        logger = std::make_shared< spdlog::async_logger >("da_global",
                                                          sinks.begin(),
                                                          sinks.end(),
                                                          spdlog::thread_pool(),
                                                          spdlog::async_overflow_policy::block);
    } else {
        logger = std::make_shared< spdlog::logger >("da_global", sinks.begin(), sinks.end());
    }

    //由于都通过DAMessageLogItem控制，因此，字需要输出内容即可
    logger->set_pattern("%v");
    //
    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::info);
    //同时每10秒flush一次
    spdlog::flush_every(std::chrono::seconds(flush_every_sec));
    spdlog::set_default_logger(logger);
    globalMessageHandleValues.setLogger(logger);
}

void _initializeRotatingSpdlog(const std::string& filename, int maxfile_size, int maxfile_counts, int flush_every_sec, bool output_stdout, bool async_logger)
{
    if (maxfile_size < 0) {
        maxfile_size = 10485760;
    }
    if (maxfile_counts < 0) {
        maxfile_counts = 1;
    }

    spdlog::filename_t filename_nor = filename;
    std::vector< spdlog::sink_ptr > sinks;

    if (output_stdout) {
        auto stdout_sink = std::make_shared< spdlog::sinks::stdout_color_sink_mt >();
        sinks.emplace_back(stdout_sink);
    }

    auto rotating_normal_sink = std::make_shared< spdlog::sinks::rotating_file_sink_mt >(filename_nor, maxfile_size, maxfile_counts);
    sinks.emplace_back(rotating_normal_sink);
    std::shared_ptr< spdlog::logger > logger;
    if (async_logger) {
        //初始化异步线程的参数
        spdlog::init_thread_pool(10240, 1);
        logger = std::make_shared< spdlog::async_logger >("da_global",
                                                          sinks.begin(),
                                                          sinks.end(),
                                                          spdlog::thread_pool(),
                                                          spdlog::async_overflow_policy::block);
    } else {
        logger = std::make_shared< spdlog::logger >("da_global", sinks.begin(), sinks.end());
    }

    //由于都通过DAMessageLogItem控制，因此，字需要输出内容即可
    logger->set_pattern("%v");
    //
    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::info);
    //同时每10秒flush一次
    spdlog::flush_every(std::chrono::seconds(flush_every_sec));
    spdlog::set_default_logger(logger);
    globalMessageHandleValues.setLogger(logger);
    // spdlog::set_automatic_registration(true);
}

/**
 * @brief 回调消息
 * @param type
 * @param context
 * @param msg
 */
void _daMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    DAMessageLogItem item(type, context, msg);
    if (type >= globalMessageHandleValues.getMsgQueueRecordMsgType()) {
        //只有type大于等于设定的msgtype才会记录到队列中
        globalMessageHandleValues.msgQueue().append(item);
    }

    if (globalMessageHandleValues.enableSpdLog()) {
        std::string dt         = item.datetimeToString().toStdString();
        std::string ms         = item.getMsg().toStdString();
        int line               = item.getLine();
        const char* file       = context.file ? context.file : "";
        const char* fun        = context.function ? context.function : "";
        spdlog::logger* logger = globalMessageHandleValues.logger();
        //        const char* pattern    = "[{0}][{1}][{2}:{3}]:{4}";
        const char* pattern = globalMessageHandleValues.getPatternChar();
        switch (type) {
        case QtDebugMsg:
            logger->debug(pattern, "debug", dt, line, fun, file, ms);
            break;
        case QtWarningMsg:
            logger->warn(pattern, "warn", dt, line, fun, file, ms);
            break;
        case QtCriticalMsg:
            logger->critical(pattern, "critical", dt, line, fun, file, ms);
            break;
        case QtFatalMsg:
            logger->error(pattern, "error", dt, line, fun, file, ms);
            break;
        case QtInfoMsg:
        default:
            logger->info(pattern, "info", dt, line, fun, file, ms);
            break;
        }
    }
}

/**
 * @brief 注销
 */
void daUnregisterMessageHandler()
{
    qInstallMessageHandler(0);
    globalMessageHandleValues.setEnableSpdLog(false);
    switch (globalMessageHandleValues.getMsgHandleType()) {
    case _DAMessageHandlerGlobalValues::MsgHandleType::HandleMsgRotateFile:
    case _DAMessageHandlerGlobalValues::MsgHandleType::HandleMsgStdout: {
        //这些需要调用spdlog
        spdlog::drop_all();
        spdlog::shutdown();
    } break;
    default:
        break;
    }
}

/**
 * @brief 注册一个旋转文件的消息处理
 *
 * 针对qDebug等内容写入文件，且文件可以进行旋转，所谓旋转既是可以指定文件大小和文件数量，日志文件永远不会超过指定的大小和数量
 *
 * 通过@sa qSetMessagePattern 可以设置输出的文本内容
 * @note 此函数在main函数调用
 * @note 需要调用@sa daUnregisterMessageHandler 释放
 * @param filename 日志文件名
 * @param maxfile_size 最大文件尺寸(byte)，默认10mb
 * @param maxfile_counts 最大文件数量，默认5个
 * @param flush_every_sec 日志定时刷新秒数，默认15秒
 * @param output_stdout 输出到stdout
 * @param async_logger 使用异步日志,默认使用
 */
void daRegisterRotatingMessageHandler(const QString& filename, int maxfile_size, int maxfile_counts, int flush_every_sec, bool output_stdout, bool async_logger)
{
    _initializeRotatingSpdlog(filename.toStdString(), maxfile_size, maxfile_counts, flush_every_sec, output_stdout, async_logger);
    globalMessageHandleValues.setEnableSpdLog(true);
    globalMessageHandleValues.setMsgHandleType(_DAMessageHandlerGlobalValues::MsgHandleType::HandleMsgRotateFile);
    qInstallMessageHandler(_daMessageHandler);
}

/**
 * @brief 注册控制台的消息处理
 * @param flush_every_sec 刷新时间
 * @param async_logger 使用异步日志,默认使用
 */
void daRegisterConsolMessageHandler(int flush_every_sec, bool async_logger)
{
    _initializeConsolSpdlog(flush_every_sec, async_logger);
    globalMessageHandleValues.setEnableSpdLog(true);
    globalMessageHandleValues.setMsgHandleType(_DAMessageHandlerGlobalValues::MsgHandleType::HandleMsgStdout);
    qInstallMessageHandler(_daMessageHandler);
}

void daSetMsgQueueRecordMsgType(QtMsgType type)
{
    globalMessageHandleValues.setMsgQueueRecordMsgType(type);
}

void daSetMessagePattern(const QString& p)
{
    globalMessageHandleValues.setPattern(p);
}

/**
 * @brief 设置消息的模板
 *
 * da消息占位符有如下：
 *
 * - {level} 对应{0} 代表是否打印消息等级，消息等级有debug/warn/critical/error/info五种
 * - {datetime} 对应{1} 代表日期
 * - {line} 对应{2} 代表打印行号
 * - {fun}  对应{3}代表打印函数名
 * - {file}  对应{4}代表打印文件名
 * - {msg}  对应{5}代表消息主体
 * @param p patter字符串，默认为[{datetime}][{line}]{msg}
 */
void _DAMessageHandlerGlobalValues::setPattern(const QString& p)
{
    //对占位符进行替换
    QString s = p;
    s.replace("{level}", "{0}");
    s.replace("{datetime}", "{1}");
    s.replace("{line}", "{2}");
    s.replace("{fun}", "{3}");
    s.replace("{file}", "{4}");
    s.replace("{msg}", "{5}");
    _pattern = s.toStdString();
}

const char* _DAMessageHandlerGlobalValues::getPatternChar() const
{
    return _pattern.c_str();
}

/**
 * @brief 禁止MessageHandler，如果针对一些qDebug、qInfo不想被捕获的，
 * 可以调用此函数，但要保证使用daEnableQtMessageHandler恢复
 */
void daDisableQtMessageHandler()
{
    globalMessageHandleValues.setEnableMessageHandle(false);
}

/**
 * @brief 开启MessageHandler
 */
void daEnableQtMessageHandler()
{
    globalMessageHandleValues.setEnableMessageHandle(true);
}

}  // namespace DA
