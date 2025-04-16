#include "DataAnalysisActions.h"
#include <QDebug>
DataAnalysisActions::DataAnalysisActions(QObject* obj) : QObject(obj)
{
	buildActions();
	retranslate();
}

DataAnalysisActions::~DataAnalysisActions()
{
}

void DataAnalysisActions::buildActions()
{
	actionSpectrum     = createAction("actionSpectrum", ":/DataAnalysis/icons/icon/spectrum.svg");
	actionFilter       = createAction("actionFilter", ":/DataAnalysis/icons/icon/filter.svg");
	actionPeakAnalysis = createAction("actionPeakAnalysis", ":/DataAnalysis/icons/icon/peak-analysis.svg");
	actionWaveletCWT   = createAction("actionWaveletAnalysis", ":/DataAnalysis/icons/icon/wavelet.svg");
}

void DataAnalysisActions::retranslate()
{
	actionSpectrum->setText(tr("Spectrum"));           // cn: 频谱
	actionFilter->setText(tr("Filter"));               // cn: 滤波
	actionPeakAnalysis->setText(tr("Peak Analysis"));  // cn: 峰值分析
	actionWaveletCWT->setText(tr("Wavelet CWT"));      // cn: 连续小波变换
}

QAction* DataAnalysisActions::createAction(const char* objname)
{
	QAction* act = new QAction(this);
	act->setObjectName(QString::fromUtf8(objname));
	recordAction(act);
	return act;
}

QAction* DataAnalysisActions::createAction(const char* objname, bool checkable, bool checked, QActionGroup* actGroup)
{
	QAction* act = createAction(objname);
	act->setCheckable(checkable);
	act->setChecked(checked);
	if (actGroup) {
		act->setActionGroup(actGroup);
	}
	return act;
}

QAction* DataAnalysisActions::createAction(const char* objname, const char* iconpath)
{
	QAction* act = createAction(objname);
	act->setIcon(QIcon(iconpath));
	return act;
}

QAction*
DataAnalysisActions::createAction(const char* objname, const char* iconpath, bool checkable, bool checked, QActionGroup* actGroup)
{
	QAction* act = createAction(objname, iconpath);
	act->setCheckable(checkable);
	act->setChecked(checked);
	if (actGroup) {
		act->setActionGroup(actGroup);
	}
	return act;
}

void DataAnalysisActions::recordAction(QAction* act)
{
	if (nullptr == act) {
		qWarning() << tr("DAAppActionsInterface::recordAction get null action");
		return;
	}
#ifdef QT_DEBUG
	if (mObjectToAction.contains(act->objectName())) {
		qWarning() << tr("DAAppActionsInterface::recordAction(QAction objname=%1) receive same object name, and the "
						 "previous record will be overwritten")
						  .arg(act->objectName());
	}
#endif
	mObjectToAction[ act->objectName() ] = act;
}
