#ifndef SENDTOKENHISTORY_H
#define SENDTOKENHISTORY_H

#include <QWidget>
QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace Ui {
class SendTokenHistory;
}

class SendTokenHistory : public QWidget
{
    Q_OBJECT

public:
    explicit SendTokenHistory(const QModelIndex &idx,QWidget *parent = 0);
    ~SendTokenHistory();

    void showVisual(bool visual);
    void updateInfo(QString status);

private:
    Ui::SendTokenHistory *ui;
};

#endif // SENDTOKENHISTORY_H
