#ifndef UNIONACCOUNTCREATE_H
#define UNIONACCOUNTCREATE_H

#include <QWidget>
#include "wallet/wallet.h"
class WalletModel;
class QSortFilterProxyModel;
namespace Ui {
class unionaccountcreate;
}

class unionaccountcreate : public QWidget
{
    Q_OBJECT

public:
    explicit unionaccountcreate(QWidget *parent = 0);
    ~unionaccountcreate();
    void setModel(WalletModel *_model);
    void showEvent(QShowEvent *event);
    void setInit();
Q_SIGNALS:
    void opensuccesscreatePage(QString s1,QString s2);
    void refreshunionaccount();
private Q_SLOTS:
    void on_createBtn_pressed();
    void coinUpdate(int idx);

    void on_btn_genkey_pressed();

private:
    Ui::unionaccountcreate *ui;
    WalletModel *model;
};

#endif // UNIONACCOUNTCREATE_H
