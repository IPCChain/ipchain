#ifndef UNIONACCOUNT_H
#define UNIONACCOUNT_H

#include <QWidget>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include "wallet/wallet.h"
class WalletModel;

namespace Ui {
class unionaccount;
}
class WalletModel;
class unionaccountcreate;
class unionaccountgenkey;
class unionaccountjoin;
class unionaccounttransaction;
class unionaccounttrasend;
class unionaccounttrasign;
class SuccessfulTrade;
class ipcSelectAddress;
class unionaccounthistory;

class unionacounthistorydetail;
class unionacounthistorydetailR;

class unionaccount : public QWidget
{
    Q_OBJECT

public:
    explicit unionaccount(QWidget *parent = 0);
    ~unionaccount();

    void setModel(WalletModel *_model);

    int addnewoneaccount(QString name,QString add,QString m_name);

    static void updatalist();
    static bool m_bNeedUpdateLater;

private Q_SLOTS:

    void unionPage_goback();

    void unionRPage_goback();
    void on_genkeyBtn_pressed();

    void on_joinuniaccBtn_pressed();

    void gotohistorysend(std::string add, CAmount fee,bool isSend,CAmount num,QString status,QString strtime,QString txid);
    void gotohistoryrecv(std::string add, CAmount fee,bool isSend,CAmount num,QString status,QString strtime,QString txid);



    void gotohistorypage(CAmount num,std::string add,std::map<uint256,const CWalletTx*> s);
    void gotosuccesscreatePage(QString s1,QString s2);
    void gotoUnionAccounttrainfo(QString s1,QString s2);
    void gotounionaccountgenkeyPage();
    void gotosuccessjoinPage();
    void gotosendPage(QString s);
    void gotosignPage();
    void gotosignsuccessPage();
    void gotosendsuccessPage(QString s1,QString s2);
    void gotoUnionAddressPage();
    void gobacktounionaccountgenkeyPage(QString address);
    void gobacktotraPage();
    void gobacktoTrainfoPage();

    void updateunionaccountList();

    void on_createuniaccBtn_pressed();

    void changelabel(QLabel*);

     void updataLater();

private:
    Ui::unionaccount *ui;

    QLabel* pOldSelectIpcButtons;

    QVBoxLayout  *pvboxlayoutall;

    QStackedWidget *walletStackBranchPage;
    unionaccountcreate*  unionaccountcreatePage;
    unionaccountgenkey*  unionaccountgenkeyPage;
    unionaccountjoin*  unionaccountjoinPage;
    unionaccounttransaction*  unionaccounttransactionPage;
    unionaccounttrasend*  unionaccounttrasendPage;
    unionaccounttrasign*  unionaccounttrasignPage;
    SuccessfulTrade* SuccessfulTradePage;
    ipcSelectAddress* unionSelectAddressPage;

    unionaccounthistory* unionaccounthistoryPage ;

    unionacounthistorydetail* unionacounthistorydetailPage ;
    unionacounthistorydetailR* unionacounthistorydetailPageR;

     WalletModel *model;

};

#endif // UNIONACCOUNT_H
