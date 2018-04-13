#ifndef IPCCONFIRMAUTHORIZATIONTRANSACTION_H
#define IPCCONFIRMAUTHORIZATIONTRANSACTION_H

#include <QWidget>
#include "walletmodel.h"
class WalletModel;

namespace Ui {
class ipcconfirmauthorizationtransaction;
}

class ipcconfirmauthorizationtransaction : public QWidget
{
    Q_OBJECT

public:
    explicit ipcconfirmauthorizationtransaction(QWidget *parent = 0);
    ~ipcconfirmauthorizationtransaction();
    void setInfo(QStringList data1);
    void setModel(WalletModel *_model);


Q_SIGNALS:
    void confirmauthorizationipc();
    void gotoSuccessfultradePage(int);
    void backtoipcauthorization();

private Q_SLOTS:
    void on_pushButton_authIPC_pressed();

    void on_pushButton_pressed();

private:
    Ui::ipcconfirmauthorizationtransaction *ui;
    WalletModel *walletModel;
};

#endif // IPCCONFIRMAUTHORIZATIONTRANSACTION_H
