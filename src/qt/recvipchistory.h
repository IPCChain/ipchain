#ifndef RECVIPCHISTORY_H
#define RECVIPCHISTORY_H

#include <QWidget>
QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE
namespace Ui {
class recvipchistory;
}

class recvipchistory : public QWidget
{
    Q_OBJECT

public:
    explicit recvipchistory(const QModelIndex &idx,QWidget *parent = 0);
    ~recvipchistory();
void showVisual(bool visual);
Q_SIGNALS:
    void backMain();
private Q_SLOTS:
    void on_pushButton_pressed();

private:
    Ui::recvipchistory *ui;
};

#endif // RECVIPCHISTORY_H
