#ifndef SETTINGWIDGET_H
#define SETTINGWIDGET_H

#include <QWidget>

namespace Ui {
class Settingwidget;
}

class Settingwidget : public QWidget
{
    Q_OBJECT

public:
    explicit Settingwidget(QWidget *parent = 0);
    ~Settingwidget();

private:
    Ui::Settingwidget *ui;
Q_SIGNALS:
    void openPasswordSetwidget(int tag);
    void openSendCoinsAffrimwidget();
private Q_SLOTS:
    void on_GoSetButton_pressed();
    void on_NoSetButton_pressed();
};

#endif // SETTINGWIDGET_H
