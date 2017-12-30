#ifndef SENDIPCHISTORY_H
#define SENDIPCHISTORY_H

#include <QWidget>
QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE
namespace Ui {
class sendipchistory;
}

class sendipchistory : public QWidget
{
    Q_OBJECT

public:
    explicit sendipchistory(const QModelIndex &idx,QWidget *parent = 0);
    ~sendipchistory();
    void showVisual(bool visual);
Q_SIGNALS:
    void backMain();
private Q_SLOTS:
    void on_pushButton_pressed();

private:
    Ui::sendipchistory *ui;
};

#endif // SENDIPCHISTORY_H
