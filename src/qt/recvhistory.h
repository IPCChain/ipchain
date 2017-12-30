#ifndef RECVHISTORY_H
#define RECVHISTORY_H

#include <QWidget>
QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE
namespace Ui {
class RecvHistory;
}

class RecvHistory : public QWidget
{
    Q_OBJECT

public:
    explicit RecvHistory(const QModelIndex &idx,QWidget *parent = 0);
    ~RecvHistory();
    void showVisual(bool visual);
Q_SIGNALS:
    void backMain();
private Q_SLOTS:
    void on_pushButton_pressed();

private:
    Ui::RecvHistory *ui;
};

#endif // RECVHISTORY_H
