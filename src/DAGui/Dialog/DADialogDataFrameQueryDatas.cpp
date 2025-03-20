#include "DADialogDataFrameQueryDatas.h"
#include "ui_DADialogDataFrameQueryDatas.h"
#include <QLineEdit>

namespace DA
{
DADialogDataFrameQueryDatas::DADialogDataFrameQueryDatas(QWidget* parent)
	: QDialog(parent), ui(new Ui::DADialogDataFrameQueryDatas)
{
	ui->setupUi(this);
}

DADialogDataFrameQueryDatas::~DADialogDataFrameQueryDatas()
{
	delete ui;
}

QList< QString > DADialogDataFrameQueryDatas::getQueryConditions()
{
	QList< QString > contents;
	// 遍历 verticalLayout_Edit 中的所有子项
	for (int i = 0; i < ui->verticalLayout_Edit->count(); ++i) {
		QLayoutItem* layoutItem = ui->verticalLayout_Edit->itemAt(i);
		if (layoutItem && layoutItem->layout()) {
			// 获取子布局（QHBoxLayout）
			QLayout* childLayout = layoutItem->layout();
			// 遍历子布局中的控件
			for (int j = 0; j < childLayout->count(); ++j) {
				QWidget* widget = childLayout->itemAt(j)->widget();
				if (widget && widget->inherits("QLineEdit")) {
					QLineEdit* lineEdit = qobject_cast< QLineEdit* >(widget);
					if (lineEdit && !lineEdit->text().isEmpty()) {
						contents.append(lineEdit->text());
					}
				}
			}
		}
	}
	return contents;
}

bool DADialogDataFrameQueryDatas::getLogicOperations()
{
	if (ui->comboBox->currentText() == "AND") {
		return true;
	} else {
		return false;
	}
}

void DADialogDataFrameQueryDatas::on_pushButton_clicked()
{
	QHBoxLayout* hLayout          = new QHBoxLayout();                // 创建一个水平布局
	QLineEdit* lineEdit           = new QLineEdit(this);              // 创建一个 QLineEdit
	QPushButton* pushButtonDelete = new QPushButton("Delete", this);  // 创建一个删除按钮

	hLayout->addWidget(lineEdit);                 // 将 QLineEdit 添加到水平布局
	hLayout->addWidget(pushButtonDelete);         // 将删除按钮添加到水平布局
	ui->verticalLayout_Edit->addLayout(hLayout);  // 将水平布局添加到垂直布局

	// 连接删除按钮的点击信号
	connect(pushButtonDelete, &QPushButton::clicked, this, [hLayout]() {
		// 删除水平布局中的所有控件
		QLayoutItem* item;
		while ((item = hLayout->takeAt(0)) != nullptr) {
			delete item->widget();  // 删除控件
			delete item;            // 删除布局项
		}
		delete hLayout;  // 删除水平布局
	});
}

}  // end DA
