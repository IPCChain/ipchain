#ifndef UNIONACCOUNTTRASEND_H
#define UNIONACCOUNTTRASEND_H

#include <QWidget>
#include "wallet/wallet.h"
#include "walletmodel.h"
class WalletModel;
namespace Ui {
class unionaccounttrasend;
}

class unionaccounttrasend : public QWidget
{
    Q_OBJECT

public:
    explicit unionaccounttrasend(QWidget *parent = 0);
    ~unionaccounttrasend();
    void setModel(WalletModel *_model);

    void setaddress(QString add);

    QString getaddress();

    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn);

Q_SIGNALS:
    void opensendsuccessPage(QString s1,QString s2);
    void backtotraPage();

private Q_SLOTS:
    void on_sendbtn_pressed();

    void on_pushButton_back_pressed();

private:
    Ui::unionaccounttrasend *ui;
    std::string m_sendcoinerror;//,m_error;
    WalletModel *model;
    QString m_add;
};

#endif // UNIONACCOUNTTRASEND_H
