#include "DAFontEditPannelWidget.h"
#include "ui_DAFontEditPannelWidget.h"
#include <QDebug>
namespace DA
{
DAFontEditPannelWidget::DAFontEditPannelWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::DAFontEditPannelWidget), _fontPointSize(10)
{
    ui->setupUi(this);
    ui->comboBoxFontSize->addItems(
            { "6", "8", "9", "10", "11", "12", "14", "16", "18", "20", "22", "24", "26", "28", "36", "48", "72" });
    ui->comboBoxFontSize->setCurrentText("10");
    ui->pushButtonColor->setColor(Qt::black);
    connect(ui->fontComboBox, &QFontComboBox::currentFontChanged, this, &DAFontEditPannelWidget::signalEmitFontChanged);
    connect(ui->pushButtonColor, &DA::DAColorPickerButton::colorChanged, this, &DAFontEditPannelWidget::currentFontColorChanged);
    connect(ui->toolButtonBold, &QToolButton::clicked, this, &DAFontEditPannelWidget::signalEmitFontChanged);
    connect(ui->toolButtonItalic, &QToolButton::clicked, this, &DAFontEditPannelWidget::signalEmitFontChanged);
    connect(ui->toolButtonUnderline, &QToolButton::clicked, this, &DAFontEditPannelWidget::signalEmitFontChanged);
    connect(ui->comboBoxFontSize, &QComboBox::currentTextChanged, this, &DAFontEditPannelWidget::onComboBoxFontSizeTextChanged);
    connect(ui->toolButtonFontSizeAdd, &QToolButton::clicked, this, &DAFontEditPannelWidget::onFontSizeAdd);
    connect(ui->toolButtonFontSizeSub, &QToolButton::clicked, this, &DAFontEditPannelWidget::onFontSizeSub);
}

DAFontEditPannelWidget::~DAFontEditPannelWidget()
{
    delete ui;
}

/**
 * @brief 设置当前字体颜色
 * @return
 */
QColor DAFontEditPannelWidget::getCurrentFontColor() const
{
    return ui->pushButtonColor->color();
}

/**
 * @brief 获取当前字体颜色
 * @param c
 */
void DAFontEditPannelWidget::setCurrentFontColor(const QColor& c)
{
    ui->pushButtonColor->setColor(c);
    //自动触发信号
}

/**
 * @brief 设置当前字体
 * @note 此函数不发射信号
 * @param f
 */
void DAFontEditPannelWidget::setCurrentFont(const QFont& f)
{
    QSignalBlocker b1(ui->toolButtonBold), b2(ui->toolButtonItalic), b3(ui->toolButtonUnderline), b4(ui->fontComboBox),
            b5(ui->comboBoxFontSize);
    ui->toolButtonBold->setChecked(f.bold());
    ui->toolButtonItalic->setChecked(f.italic());
    ui->toolButtonUnderline->setChecked(f.underline());
    ui->fontComboBox->setCurrentFont(f);
    ui->comboBoxFontSize->setCurrentText(QString::number((f.pointSize() < 0) ? f.pixelSize() : f.pointSize()));
}

/**
 * @brief 获取当前字体
 * @return
 */
QFont DAFontEditPannelWidget::getCurrentFont()
{
    QFont f = ui->fontComboBox->currentFont();
    f.setBold(ui->toolButtonBold->isChecked());
    f.setItalic(ui->toolButtonItalic->isChecked());
    f.setUnderline(ui->toolButtonUnderline->isChecked());
    f.setPointSize(_fontPointSize);
    return f;
}

/**
 * @brief 设置字体大小
 * @param w
 */
void DAFontEditPannelWidget::setFontWeight(int w)
{
    ui->comboBoxFontSize->setCurrentText(QString::number(w));
    _fontPointSize = w;
    signalEmitFontChanged();
}

/**
 * @brief 获取字体大小
 * @return
 */
int DAFontEditPannelWidget::getFontWeight() const
{
    return ui->comboBoxFontSize->currentText().toInt();
}

void DAFontEditPannelWidget::onFontComboBoxCurrentFontChanged(const QFont& f)
{
    Q_UNUSED(f);
    signalEmitFontChanged();
}

/**
 * @brief DAFontEditPannelWidget::onComboBoxFontSizeTextChanged
 * @param t
 */
void DAFontEditPannelWidget::onComboBoxFontSizeTextChanged(const QString& t)
{
    bool isok;
    int v = t.toInt(&isok);
    if (!isok) {
        QSignalBlocker bl(ui->comboBoxFontSize);
        ui->comboBoxFontSize->setCurrentText(QString::number(_fontPointSize));
        return;
    }
    _fontPointSize = v;
    signalEmitFontChanged();
}

void DAFontEditPannelWidget::onFontSizeAdd()
{
    _fontPointSize += 1;
    QSignalBlocker bl(ui->comboBoxFontSize);
    ui->comboBoxFontSize->setCurrentText(QString::number(_fontPointSize));
    signalEmitFontChanged();
}

void DAFontEditPannelWidget::onFontSizeSub()
{
    _fontPointSize -= 1;
    if (_fontPointSize <= 0) {
        _fontPointSize = 1;
    }
    QSignalBlocker bl(ui->comboBoxFontSize);
    ui->comboBoxFontSize->setCurrentText(QString::number(_fontPointSize));
    signalEmitFontChanged();
}

void DAFontEditPannelWidget::signalEmitFontChanged()
{
    qDebug() << "signalEmitFontChanged";
    QFont f = getCurrentFont();
    emit currentFontChanged(f);
}

}
