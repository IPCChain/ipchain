#ifndef UNIONACOUNTHISTORYDETAIL_H
#define UNIONACOUNTHISTORYDETAIL_H

#include <QWidget>
#include "amount.h"
class WalletModel;
namespace Ui {
class unionacounthistorydetail;
}

class unionacounthistorydetail : public QWidget
{
    Q_OBJECT

public:
    explicit unionacounthistorydetail(QWidget *parent = 0);
    ~unionacounthistorydetail();
    void onmessagetra(QString &status);
    void setModel(WalletModel *_model);
    void setinfo(std::string add, CAmount fee,bool isSend,CAmount num,QString status,QString strtime,QString txid);
Q_SIGNALS:
    void unionPage_back();

private Q_SLOTS:
    void on_btn_back_pressed();

private:
    Ui::unionacounthistorydetail *ui;
    QString m_txid;
    WalletModel* m_walletmodel;
};

#endif // UNIONACOUNTHISTORYDETAIL_H
