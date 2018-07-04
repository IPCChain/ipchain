#ifndef ECOINCONFIRMSENDAFFRIMDIALOG_H
#define ECOINCONFIRMSENDAFFRIMDIALOG_H

#include <QWidget>
#include "walletmodel.h"

class WalletModel;

namespace Ui {
class ecoinconfirmsendaffrimdialog;
}

class ecoinconfirmsendaffrimdialog : public QWidget
{
    Q_OBJECT

public:
    explicit ecoinconfirmsendaffrimdialog(QWidget *parent = 0);
    ~ecoinconfirmsendaffrimdialog();
    std::string m_error;
    void setModel(WalletModel *_model);
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn);
    void settrainfo(QString num,QString add);
Q_SIGNALS:
    void SendeCoinSuccess(QString s1);
    void backtoecoinsend();
private Q_SLOTS:
    void on_pushButton_send_pressed();

    void on_pushButton_back_pressed();

private:
    Ui::ecoinconfirmsendaffrimdialog *ui;
    WalletModel *walletmodel;
   // WalletModelTransaction *m_tra;

};

#endif // ECOINCONFIRMSENDAFFRIMDIALOG_H
