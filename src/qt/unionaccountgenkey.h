#ifndef UNIONACCOUNTGENKEY_H
#define UNIONACCOUNTGENKEY_H

#include <QWidget>
#include "wallet/wallet.h"
class WalletModel;
namespace Ui {
class unionaccountgenkey;
}

class unionaccountgenkey : public QWidget
{
    Q_OBJECT

public:
    explicit unionaccountgenkey(QWidget *parent = 0);
    ~unionaccountgenkey();
    void setModel(WalletModel *_model);

    void setaddress(QString address);

    void clearinfo();
    void clearData();
    void showEvent(QShowEvent *event);

Q_SIGNALS:
    void opensuccessgenkeyPage();
    void selectaddress();
private Q_SLOTS:
    void on_genkey_pressed();

    void on_selectadd_btn_pressed();

private:
    Ui::unionaccountgenkey *ui;
    WalletModel *model;
    QString m_address;
};

#endif // UNIONACCOUNTGENKEY_H
