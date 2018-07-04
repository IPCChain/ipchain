#ifndef UNIONACCOUNTTRASIGN_H
#define UNIONACCOUNTTRASIGN_H

#include <QWidget>
#include "wallet/wallet.h"
#include "cmessagebox.h"
class WalletModel;
namespace Ui {
class unionaccounttrasign;
}

class unionaccounttrasign : public QWidget
{
    Q_OBJECT

public:
    explicit unionaccounttrasign(QWidget *parent = 0);
    ~unionaccounttrasign();
    void setModel(WalletModel *_model);
    void set0Text();
    bool getSignInfo();
    void setSourceAddress(std::string address){m_strSourceaddress = address;}
Q_SIGNALS:
    void backtotraPage();
    void opensignsuccessPage();
private Q_SLOTS:
    void on_signBtn_pressed();

    void on_backbtn_pressed();

    void on_btn_sign_pressed();

private:
    void setMsgDlgPlace(CMessageBox* pmsgdlg);
    Ui::unionaccounttrasign *ui;
    WalletModel *model;
    std::string m_strSourceaddress;
};

#endif // UNIONACCOUNTTRASIGN_H
