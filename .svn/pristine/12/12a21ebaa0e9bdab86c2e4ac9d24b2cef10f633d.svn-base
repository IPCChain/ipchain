#ifndef IPCCONFIRMTRANSFERTRANSACTION_H
#define IPCCONFIRMTRANSFERTRANSACTION_H

#include <QWidget>
#include "walletmodel.h"
class WalletModel;

namespace Ui {
class ipcconfirmtransfertransaction;
}

class ipcconfirmtransfertransaction : public QWidget
{
    Q_OBJECT

public:
    explicit ipcconfirmtransfertransaction(QWidget *parent = 0);
    ~ipcconfirmtransfertransaction();
    void setModel(WalletModel *_model);
    void setInfo(QString name,QString address);
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn);


Q_SIGNALS:
    void confirmtransferipc();
    void gotoSuccessfultradePage(int);
    void backtoipctransfertransaction();

private Q_SLOTS:
    void on_pushButton_confirmtransferipc_pressed();

    void on_pushButton_back_pressed();

private:
    Ui::ipcconfirmtransfertransaction *ui;
    WalletModel *walletModel;
};

#endif // IPCCONFIRMTRANSFERTRANSACTION_H
