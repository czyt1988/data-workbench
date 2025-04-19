﻿#ifndef DADATAMANAGERCOMBOBOX_H
#define DADATAMANAGERCOMBOBOX_H
#include "ctkTreeComboBox.h"
#include "DAGuiAPI.h"
#include "DADataManager.h"
namespace DA
{

/**
 * @brief 这是一个树形结构的combobox，以树形展开DataManager
 */
class DAGUI_API DADataManagerComboBox : public ctkTreeComboBox
{
	Q_OBJECT
	DA_DECLARE_PRIVATE(DADataManagerComboBox)
public:
	DADataManagerComboBox(QWidget* par = nullptr);
	~DADataManagerComboBox();
	virtual void showPopup() override;

public:
	// 设置DADataManager，combobox不管理DADataManager的内存
	void setDataManager(DADataManager* dmgr);
	DADataManager* getDataManager() const;
	// 获取当前的Data
	DAData getCurrentDAData() const;
	// 设置当前选中的data
	void setCurrentDAData(const DAData& d);
	// 是否把dataframe下的series也展示,默认为true
	void setShowSeriesUnderDataframe(bool on);
	bool isShowSeriesUnderDataframe() const;

private slots:
	void onCurrentIndexChanged(const QString& text);
	void onDataChanged(const DA::DAData& d, DA::DADataManager::ChangeType t);
Q_SIGNALS:
	/**
	 * @brief dataframe的series改变
	 * @param data Dataframe
	 * @param seriesName 系列名 如果选中了dataframe，此参数为QString()
	 */
	void currentDataframeSeriesChanged(const DA::DAData& data, const QString& seriesName);
};
}

#endif  // DADATAMANAGERCOMBOBOX_H
