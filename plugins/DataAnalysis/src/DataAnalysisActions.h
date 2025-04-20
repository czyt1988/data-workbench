#ifndef DATAANALYSISACTIONS_H
#define DATAANALYSISACTIONS_H
#include <QAction>
#include <QHash>
/**
 * @brief 这里管理数据分析的所有action
 */
class DataAnalysisActions : public QObject
{
public:
	DataAnalysisActions(QObject* obj = nullptr);
	~DataAnalysisActions();
	// 重新翻译
	void retranslate();
	// 创建一个action,并管理
	QAction* createAction(const char* objname);
	QAction* createAction(const char* objname, bool checkable, bool checked = false, QActionGroup* actGroup = nullptr);
	QAction* createAction(const char* objname, const char* iconpath);
	QAction* createAction(const char* objname,
						  const char* iconpath,
						  bool checkable,
						  bool checked           = false,
						  QActionGroup* actGroup = nullptr);
	// 记录action，action要保证有独立的object name
	void recordAction(QAction* act);

public:
	QAction* actionSpectrum;      ///< 频谱
	QAction* actionFilter;        ///< 滤波
	QAction* actionPeakAnalysis;  ///< 峰值分析
	QAction* actionWaveletCWT;    ///< 连续小波变换
	QAction* actionWaveletDWT;    ///< 离散小波变换

	QHash< QString, QAction* > mObjectToAction;

private:
	// 创建actions
	void buildActions();
};

#endif  // DATAANALYSISACTIONS_H
