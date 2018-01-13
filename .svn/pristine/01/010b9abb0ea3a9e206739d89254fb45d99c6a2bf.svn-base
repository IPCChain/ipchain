#ifndef RECVTOKENHISTORY_H
#define RECVTOKENHISTORY_H

#include <QWidget>
QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE
namespace Ui {
class RecvTokenHistory;
}

class RecvTokenHistory : public QWidget
{
    Q_OBJECT

public:
    explicit RecvTokenHistory(const QModelIndex &idx,QWidget *parent = 0);
    ~RecvTokenHistory();
    void showVisual(bool visual);
    void updateInfo(QString status);
private:
    Ui::RecvTokenHistory *ui;
};

#endif // RECVTOKENHISTORY_H
