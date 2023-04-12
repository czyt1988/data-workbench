#ifndef DADATAMANAGERCOMBOBOX_H
#define DADATAMANAGERCOMBOBOX_H
#include <QComboBox>
#include "DAGuiAPI.h"
#include "DADataManager.h"
namespace DA
{
DA_IMPL_FORWARD_DECL(DADataManagerComboBox)
/**
 * @brief 这是一个树形结构的combobox，以树形展开DataManager
 */
class DAGUI_API DADataManagerComboBox : public QComboBox
{
    Q_OBJECT
    DA_IMPL(DADataManagerComboBox)
public:
    DADataManagerComboBox(QWidget* par = nullptr);
    ~DADataManagerComboBox();
    virtual void showPopup() override;

public:
    //设置DADataManager，combobox不管理DADataManager的内存
    void setDataManager(DADataManager* dmgr);
    DADataManager* getDataManager() const;
};
}

#endif  // DADATAMANAGERCOMBOBOX_H
