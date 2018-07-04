#ifndef UNIONACCOUNTJOIN_H
#define UNIONACCOUNTJOIN_H

#include <QWidget>
#include "wallet/wallet.h"
class WalletModel;
namespace Ui {
class unionaccountjoin;
}

class unionaccountjoin : public QWidget
{
    Q_OBJECT

public:
    explicit unionaccountjoin(QWidget *parent = 0);
    ~unionaccountjoin();
    void setModel(WalletModel *_model);
    void showEvent(QShowEvent *event);
Q_SIGNALS:
    void opensuccessjoinPage();
    void refreshunionaccount();
private Q_SLOTS:
    void on_joinBtn_pressed();

    void on_btn_import_pressed();
    
private:
    Ui::unionaccountjoin *ui;
    WalletModel *model;
};

#endif // UNIONACCOUNTJOIN_H
