#ifndef {{PLUGIN-BASE-NAME}}UI_H
#define {{PLUGIN-BASE-NAME}}UI_H
#include <QObject>
#include <QAction>

namespace DA
{
class DACoreInterface;
class DAUIInterface;
class DAActionsInterface;
}

class {{plugin-base-name}}UI : public QObject
{
	Q_OBJECT
public:
	explicit {{plugin-base-name}}UI(QObject* par = nullptr);
	virtual ~{{plugin-base-name}}UI() override;
	// 初始化
	bool initialize(DA::DACoreInterface* core);
	// 清理initialize创建的全部UI资源（插件热卸载finalize时调用）
	void finalize();
	// 翻译
	void retranslateUi();
public:
	//这里你可以用于构建你的界面，保存你的action
private:
    DA::DACoreInterface* mCore { nullptr };
	DA::DAUIInterface* mUi{nullptr};
	DA::DAActionsInterface* mActions { nullptr };
};

#endif  // {{PLUGIN-BASE-NAME}}PLUGIN_H
