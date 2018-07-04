#ifndef UNIONACCOUNTHISTORY_H
#define UNIONACCOUNTHISTORY_H
#include <QVBoxLayout>
#include <QWidget>
#include "wallet/wallet.h"
class WalletModel;
class CWalletTx;
namespace Ui {
class unionaccounthistory;
}
struct tra_info{
   bool isSend;
   std::string add;
   CAmount num;
   CAmount fee;
   QString status;
   QString strtime ;
   QString txid;
};
class unionaccounthistory : public QWidget
{
    Q_OBJECT

public:
     unionaccounthistory(QWidget *parent = 0);
     ~unionaccounthistory();
     void updateinfo(std::map<uint256,const CWalletTx*> s);
     void setAdd(std::string add);
     void setnum(CAmount num);
     void addline();
     void setModel(WalletModel *_model);
     Q_SIGNALS:
     void backtoTraPage();
     void jumptohistorysend(std::string add, CAmount fee,bool isSend,CAmount num,QString status,QString strtime,QString txid);
     void jumptohistoryrecv(std::string add, CAmount fee,bool isSend,CAmount num,QString status,QString strtime,QString txid);

private Q_SLOTS:
     void on_btn_back_pressed();
     void lookupinfodetail(std::string add, bool isSend,QString status,
                           QString strtime,CAmount s,CAmount s1,QString txid);
private:
     std::string getAdd();
     void addinfo(std::string add, CAmount fee,bool isSend,
                  CAmount num,QString status,QString strtime,QString txid);
     CAmount getnum();
     Ui::unionaccounthistory *ui;
     std::string m_add;
     CAmount m_num;
     QVBoxLayout * pvboxlayoutall;
     WalletModel * m_pwalletmodel;
     std::vector<tra_info> m_tra;
};

#endif // UNIONACCOUNTHISTORY_H
