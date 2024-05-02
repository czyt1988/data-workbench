#include "DATextReadWriter.h"
#include <QMutexLocker>
#include <QTextStream>
#include <limits>
#include <atomic>
#include <QDebug>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringEncoder>
#endif
namespace DA
{

class DATextReadWriter::PrivateData
{
	DA_DECLARE_PUBLIC(DATextReadWriter)
public:
	PrivateData(DATextReadWriter* p);
	~PrivateData();

public:
	std::unique_ptr< QFile > file;
	int totalReadLineCount { -1 };  ///< 设置总共读取的行数，当读取到达此行后会发射readAllComplete
	/**
	 * @brief 设置一次读取的行数，当开始读取时达到这个行数后会发射readComplete，
	 * 继续往下读取（没有超过m_totalReadLineCount的行数），再次达到m_readOnceLineCount设定值后发射readComplete，
	 * 继续往下读取，直到到达m_totalReadLineCount的行数(发射readAllComplete)或文件的末尾(发射reachTextEnd和readAllComplete)
	 */
	int readOnceLineCount { 500 };
	int totalReadCharCount { -1 };  ///< 设置总共读取的字符数，当读取到达此字符数后会发射readAllComplete
	/**
	 * @brief 设置一次读取的字符数，当开始读取时达到这个字符数后会发射readComplete，
	 * 继续往下读取（没有超过m_totalReadCharCount的字符数），再次达到m_readOnceCharCount设定值后发射readComplete，
	 * 继续往下读取，直到到达m_totalReadLineCount的字符数(发射readAllComplete)或文件的末尾(发射reachTextEnd和readAllComplete)
	 */
	int readOnceCharCount { -1 };
	QString codec { "UTF-8" };                          ///<  编码
	DATextReadWriter::StringFun stringFun { nullptr };  ///< 处理每次读取的函数指针，默认为nullptr
	std::atomic< bool > stopFlag { false };
	std::size_t skipHeader { 0 };
};

DATextReadWriter::PrivateData::PrivateData(DATextReadWriter* p) : q_ptr(p)
{
	file = std::make_unique< QFile >();
}

DATextReadWriter::PrivateData::~PrivateData()
{
}

//===============================================================
// DATextReadWriter
//===============================================================

DATextReadWriter::DATextReadWriter(QObject* parent) : QObject(parent), DA_PIMPL_CONSTRUCT
{
	connect(this, &DATextReadWriter::startedReadText, this, &DATextReadWriter::onStartReadText);
}

DATextReadWriter::~DATextReadWriter()
{
	qDebug() << "DATextReadWriter::~DATextReadWriter";
}

void DATextReadWriter::setFileName(const QString& name)
{
	d_ptr->file->setFileName(name);
}
///
/// \brief 总共读取的行数，当读取到达此行后会发射readAllComplete
/// \return 返回-1代表没有设置，此时会一直度到文本末尾
///
int DATextReadWriter::getTotalReadLineCount() const
{
	return d_ptr->totalReadLineCount;
}
///
/// \brief 设置总共读取的行数，当读取到达此行后会发射readAllComplete
/// \param -1代表没有设置，此时会一直度到文本末尾
///
void DATextReadWriter::setTotalReadLineCount(int totalReadLineCount)
{
	d_ptr->totalReadLineCount = totalReadLineCount;
}
///
/// \brief 一次读取的行数，当开始读取时达到这个行数后会发射readComplete，
/// 继续往下读取（没有超过m_totalReadLineCount的行数），再次达到m_readOnceLineCount设定值后发射readComplete，
/// 继续往下读取，直到到达m_totalReadLineCount的行数(发射readAllComplete)或文件的末尾(发射reachTextEnd和readAllComplete)
/// \return 返回-1代表没有设置，此时会读取所有到文件末尾才发射一次readComplete
/// \note 默认为500
///
int DATextReadWriter::getReadOnceLineCount() const
{
	return d_ptr->readOnceLineCount;
}
///
/// \brief 设置一次读取的行数，但开始读取时达到这个行数后会发射readComplete，
/// 继续往下读取（没有超过m_totalReadLineCount的行数），再次达到m_readOnceLineCount设定值后发射readComplete，
/// 继续往下读取，直到到达m_totalReadLineCount的行数(发射readAllComplete)或文件的末尾(发射reachTextEnd和readAllComplete)
/// \param readOnceLineCount -1代表没有设置，此时会读取所有到文件末尾才发射一次readComplete
/// \note 默认为500
///
void DATextReadWriter::setReadOnceLineCount(int readOnceLineCount)
{
	d_ptr->readOnceLineCount = readOnceLineCount;
}
///
/// \brief 总共读取的字符数，当读取到达此字符数后会发射readAllComplete
/// \return 返回-1代表没有设置，此时会读取所有到文件末尾才发射一次readComplete
///
int DATextReadWriter::getReadOnceCharCount() const
{
	return d_ptr->readOnceCharCount;
}
///
/// \brief 设置总共读取的字符数，当读取到达此字符数后会发射readAllComplete
/// \param readOnceCharCount -1代表没有设置，此时会读取所有到文件末尾才发射一次readComplete
///
void DATextReadWriter::setReadOnceCharCount(int readOnceCharCount)
{
    d_ptr->readOnceCharCount = readOnceCharCount;
}

/**
 * @brief 设置跳过的行数
 * 此函数仅仅对读取有效
 * @param v
 */
void DATextReadWriter::setSkipHeader(size_t v)
{
    d_ptr->skipHeader = v;
}

int DATextReadWriter::getSkipHeader() const
{
    return static_cast< int >(d_ptr->skipHeader);
}

///
/// \brief 一次读取的字符数，当开始读取时达到这个字符数后会发射readComplete，
/// 继续往下读取（没有超过m_totalReadCharCount的字符数），再次达到m_readOnceCharCount设定值后发射readComplete，
/// 继续往下读取，直到到达m_totalReadLineCount的字符数(发射readAllComplete)或文件的末尾(发射reachTextEnd和readAllComplete)
/// \return 返回-1代表没有设置
///
int DATextReadWriter::getTotalReadCharCount() const
{
	return d_ptr->totalReadCharCount;
}
///
/// \brief 设置一次读取的字符数，当开始读取时达到这个字符数后会发射readComplete，
/// 继续往下读取（没有超过m_totalReadCharCount的字符数），再次达到m_readOnceCharCount设定值后发射readComplete，
/// 继续往下读取，直到到达m_totalReadLineCount的字符数(发射readAllComplete)或文件的末尾(发射reachTextEnd和readAllComplete)
/// \param -1代表没有设置
///
void DATextReadWriter::setTotalReadCharCount(int totalReadCharCount)
{
	d_ptr->totalReadCharCount = totalReadCharCount;
}
///
/// \brief 文件是否已经打开
/// \return
///
bool DATextReadWriter::isOpen() const
{
	return d_ptr->file->isOpen();
}
///
/// \brief 打开文件
/// \param mode QIODevice::OpenMode
/// \return
/// \see QFile::open
///
bool DATextReadWriter::open(QIODevice::OpenMode mode)
{
	if (d_ptr->file->open(mode)) {
		return true;
	}
	qDebug() << tr("can not open %1,because %2").arg(d_ptr->file->fileName()).arg(d_ptr->file->errorString());
	// int code = static_cast< DATextReadWriter::ErrorCode >(d_ptr->file->error());
	// emit occurError(code, d_ptr->file->errorString());
	return false;
}
///
/// \brief 最后一次错误信息
/// \return
///
QString DATextReadWriter::getLastErrorString() const
{
	return d_ptr->file->errorString();
}

QString DATextReadWriter::getCodec() const
{
	return d_ptr->codec;
}

void DATextReadWriter::setCodec(const QString& codec)
{
	d_ptr->codec = codec;
}
///
/// \brief 处理每次读取的函数指针，默认为nullptr
/// \param fun
///
void DATextReadWriter::setStringFun(const DATextReadWriter::StringFun& fun)
{
	d_ptr->stringFun = fun;
}
///
/// \brief 开始读取文本
/// 可以在主线程直接调用而不阻塞
///
void DATextReadWriter::startReadText()
{
	d_ptr->stopFlag = true;
	emit startedReadText();
}
///
/// \brief 插入文本
///
/// \param text
///
///
void DATextReadWriter::appendText(const QString& text)
{
	if (!isOpen()) {
		if (!open(QIODevice::ReadWrite | QIODevice::Append | QIODevice::Text)) {
			return;
		}
	}
	QTextStream stream(d_ptr->file.get());
	stream << text;
}
///
/// \brief 插入一行文本
/// \param text
///
void DATextReadWriter::appendLine(const QString& text)
{
	if (!isOpen()) {
		if (!open(QIODevice::ReadWrite | QIODevice::Append | QIODevice::Text)) {
			return;
		}
	}
	QTextStream stream(d_ptr->file.get());
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	stream << text << endl;
#else
	stream << text << Qt::endl;
#endif
}
///
/// \brief Flushes any buffered data to the file. Returns true if successful; otherwise returns false.
/// \return
///
bool DATextReadWriter::flush()
{
	if (d_ptr->file->flush())
		return true;
	return false;
}

///
/// \brief 传入一个操作QFile的函数指针，此函数指针用于操作内部保存的QFile
/// \param fun
///
void DATextReadWriter::operatorFile(std::function< void(QFile*) > fun)
{
	if (fun) {
		fun(d_ptr->file.get());
	}
}

void DATextReadWriter::onStartReadText()
{
	d_ptr->stopFlag = false;
	if (!isOpen()) {
		if (!open(QIODevice::ReadOnly | QIODevice::Text)) {
			emit finished(d_ptr->file->error());
			return;
		}
	}
	QTextStream stream(d_ptr->file.get());

	const int readOnceCharCount  = d_ptr->readOnceCharCount > 0 ? d_ptr->readOnceCharCount : 0;
	const int totalReadCharCount = d_ptr->totalReadCharCount > 0 ? d_ptr->totalReadCharCount
																 : std::numeric_limits< int >::max();
	const int readOnceLineCount  = d_ptr->readOnceLineCount > 0 ? d_ptr->readOnceLineCount
																: std::numeric_limits< int >::max();
	const int totalReadLineCount = d_ptr->totalReadLineCount > 0 ? d_ptr->totalReadLineCount
																 : std::numeric_limits< int >::max();
	const std::size_t skipHeader = d_ptr->skipHeader;
	// 这里readOnceCharCount < totalReadCharCount,readOnceLineCount < totalReadLineCount,所有下面循环的判断中会先判断小的一方
	// 也就是先判断readOnceCharCount，再判断totalReadCharCount，再判读readOnceLineCount，最后totalReadLineCount

	int currentReadLines = 0;
	int totalReadLines   = 0;
	StringFun& fun       = d_ptr->stringFun;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	stream.setCodec(d_ptr->codec.toLocal8Bit().data());
#else
	stream.setAutoDetectUnicode(true);
	auto codec = QStringConverter::encodingForName(d_ptr->codec.toLocal8Bit().data());
	if (codec) {
		stream.setEncoding(codec.value());
	} else {
		stream.setEncoding(QStringConverter::System);
	}
#endif
	if (d_ptr->stopFlag) {
		emit finished(static_cast< int >(UserTerminate));
		return;
	}

	stream.seek(0);
	QString currentReadString;
	std::size_t readLineCnt = 0;
	while (!stream.atEnd()) {
		if (d_ptr->stopFlag) {
			emit finished(static_cast< int >(UserTerminate));
			return;
		}
		QString readString = stream.readLine(readOnceCharCount);
		++readLineCnt;
		if (readLineCnt <= skipHeader) {
			continue;
		}
		currentReadString.append(readString);
		if (readString.size() != readOnceCharCount) {
			currentReadString.append("\n");
		}
		++currentReadLines;
		++totalReadLines;
		// 1. 先判断字符，字符判断从总设定字符数判读开始，总的字符数没超越，判断单次字符数
		if (currentReadString.size() >= totalReadCharCount && totalReadCharCount > 0) {
			// 说明读取的总字节长度超过设定,截取正好的长度
			currentReadString = currentReadString.left(totalReadCharCount);
			if (fun) {
				currentReadString = fun(currentReadString);
			}
			emit readComplete(currentReadString, false);
			emit finished(static_cast< int >(NoError));
			return;
		}

		// 说明读取的字符数超过了设定的单次读取的字符数限定，这时要把后段截取
		while (currentReadString.size() >= readOnceCharCount && readOnceCharCount > 0) {
			QString tmp = currentReadString.left(readOnceCharCount);
			if (fun) {
				tmp = fun(tmp);
			}
			emit readComplete(tmp, false);
			currentReadString = currentReadString.right(currentReadString.size() - readOnceCharCount);
		}

		if (0 == currentReadString.size()) {
			continue;
		}
		// 2. 这时currentReadString的长度肯度小于readOnceCharCount和totalReadCharCount
		//  开始判读行数

		if (totalReadLines >= totalReadLineCount && totalReadLineCount > 0) {
			// 说明读取的行数超过设定
			if (fun) {
				currentReadString = fun(currentReadString);
			}
			emit readComplete(currentReadString, true);
			emit finished(static_cast< int >(NoError));
			return;
		}
		// 达到单次设定的读取行数
		if (currentReadLines > readOnceLineCount && readOnceLineCount > 0) {
			if (fun) {
				currentReadString = fun(currentReadString);
			}
			emit readComplete(currentReadString, false);
			currentReadString = "";
			currentReadLines  = 0;
		}
	}

	if (0 != currentReadString.size()) {
		emit readComplete(currentReadString, true);
	}
	emit finished(static_cast< int >(NoError));
}

void DATextReadWriter::setStopRead(bool stopRead)
{
    d_ptr->stopFlag = stopRead;
}

/**
 * @brief 错误码转换为文字
 * @param c
 * @return
 */
QString DATextReadWriter::errorCodeToString(ErrorCode c)
{
	switch (c) {
	case NoError:
		return QObject::tr("No error occurred");
	case ReadError:
		return QObject::tr("An error occurred when reading from the file");
	case WriteError:
		return QObject::tr("An error occurred when writing to the file");
	case FatalError:
		return QObject::tr("A fatal error occurred");
	case ResourceError:
		return QObject::tr("Out of resources (eg, too many open files, out of memory, etc)");
	case OpenError:
		return QObject::tr("The file could not be opened");
	case AbortError:
		return QObject::tr("The operation was aborted");
	case TimeOutError:
		return QObject::tr("A timeout occurred");
	case UnspecifiedError:
		return QObject::tr("An unspecified error occurred");
	case RemoveError:
		return QObject::tr("The file could not be removed");
	case RenameError:
		return QObject::tr("The file could not be renamed");
	case PositionError:
		return QObject::tr("The position in the file could not be changed");
	case ResizeError:
		return QObject::tr("The file could not be resized");
	case PermissionsError:
		return QObject::tr("The file could not be accessed");
	case CopyError:
		return QObject::tr("The file could not be copied");
	case UserTerminate:
		return QObject::tr("User Terminate");
	default:
		break;
	}
	return QObject::tr("Unknow Error");
}

}
