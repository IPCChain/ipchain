#ifndef UNIONACOUNTHISTORYDETAILR_H
#define UNIONACOUNTHISTORYDETAILR_H

#include <QWidget>
#include "amount.h"
namespace Ui {
class unionacounthistorydetailR;
}
class WalletModel;
class unionacounthistorydetailR : public QWidget
{
    Q_OBJECT

public:
    explicit unionacounthistorydetailR(QWidget *parent = 0);
    ~unionacounthistorydetailR();
    void onmessagetra(QString &status);
    void setinfo(std::string add, CAmount fee,bool isSend,CAmount num,QString status,QString strtime,QString txid);
    void setModel(WalletModel *_model){m_walletmodel = _model;}
Q_SIGNALS:
    void unionRPage_back();
private Q_SLOTS:
    void on_btn_back_pressed();

private:
    Ui::unionacounthistorydetailR *ui;
    QString m_txid;
    WalletModel* m_walletmodel;
};

#endif // UNIONACOUNTHISTORYDETAILR_H
