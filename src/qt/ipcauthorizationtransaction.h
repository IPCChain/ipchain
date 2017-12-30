#ifndef IPCAUTHORIZATIONTRANSACTION_H
#define IPCAUTHORIZATIONTRANSACTION_H

#include <QWidget>
#include <QShowEvent>
class WalletModel;
namespace Ui {
class IpcAuthorizationTransaction;
}

class IpcAuthorizationTransaction : public QWidget
{
    Q_OBJECT

public:
    explicit IpcAuthorizationTransaction(QWidget *parent = 0);
    ~IpcAuthorizationTransaction();
    WalletModel *walletModel;
    void setModel(WalletModel * model){walletModel = model;}
    int strTimeToInt(QString);
    void showEvent(QShowEvent *);
    void SetStartTime(QString timestr){m_strStringTime = timestr;}
    void SetNameDisplay(QString s1);
private Q_SLOTS:
    void on_pushButton_confirm_pressed();
    void on_pushButton_back_pressed();
Q_SIGNALS:
    void back();
    void gotoSuccessfultradePage(int);
private:
    Ui::IpcAuthorizationTransaction *ui;
    QString m_strStringTime;
};

#endif // IPCAUTHORIZATIONTRANSACTION_H
