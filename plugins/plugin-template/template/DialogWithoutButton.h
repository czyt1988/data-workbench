#ifndef DIALOGWITHBUTTON_H
#define DIALOGWITHBUTTON_H

#include <QDialog>

namespace Ui {
class DialogWithButton;
}

class DialogWithButton : public QDialog
{
    Q_OBJECT

public:
    explicit DialogWithButton(QWidget *parent = nullptr);
    ~DialogWithButton();

private:
    Ui::DialogWithButton *ui;
};

#endif // DIALOGWITHBUTTON_H
