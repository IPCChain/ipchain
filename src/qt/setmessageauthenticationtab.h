#ifndef SETMESSAGEAUTHENTICATIONTAB_H
#define SETMESSAGEAUTHENTICATIONTAB_H

#include <QWidget>
class WalletModel;
namespace Ui {
class SetMessageAuthenticationTab;
}

class SetMessageAuthenticationTab : public QWidget
{
    Q_OBJECT

public:
    explicit SetMessageAuthenticationTab(QWidget *parent = 0);
    ~SetMessageAuthenticationTab();
    WalletModel *walletModel;
    void setModel(WalletModel * model){walletModel = model;}

private Q_SLOTS:
    void on_pushButton_check_pressed();

private:
    Ui::SetMessageAuthenticationTab *ui;
};

#endif // SETMESSAGEAUTHENTICATIONTAB_H
