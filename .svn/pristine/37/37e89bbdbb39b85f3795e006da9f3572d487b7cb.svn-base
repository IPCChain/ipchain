#ifndef UNIONACCOUNTTRANSACTION_H
#define UNIONACCOUNTTRANSACTION_H

#include <QWidget>
#include "wallet/wallet.h"
class WalletModel;
namespace Ui {
class unionaccounttransaction;
}

class unionaccounttransaction : public QWidget
{
    Q_OBJECT

public:
    explicit unionaccounttransaction(QWidget *parent = 0);
    ~unionaccounttransaction();
    void setModel(WalletModel *_model);
    void setinfo(QString s1,QString s2,CAmount s3,std::string s4,QString confirmnum,QString allmnum);
    std::string getAddress();
    void timerEvent( QTimerEvent *event );



Q_SIGNALS:
    void gosendPage(QString s);
    void gosignPage();
    void jumptohistorypage(CAmount num,std::string add, std::map<uint256,const CWalletTx*> s);
private Q_SLOTS:
    void on_signbtn_pressed();
    void on_sendbtn_pressed();

    void on_btn_history_pressed();
    
    void on_btn_export_pressed();

private:
    Ui::unionaccounttransaction *ui;
    WalletModel *model;
    QString m_addfrom;
    int m_nTimerId;
};

#endif // UNIONACCOUNTTRANSACTION_H
