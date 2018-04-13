#ifndef ECOINCONFIRMCREATEDIALOG_H
#define ECOINCONFIRMCREATEDIALOG_H

#include <QWidget>
#include "walletmodel.h"
//#include "walletmodeltransaction.h"
class WalletModel;
//class WalletModelTransaction;

namespace Ui {
class ecoinconfirmcreatedialog;
}

class ecoinconfirmcreatedialog : public QWidget
{
    Q_OBJECT

public:
    explicit ecoinconfirmcreatedialog(QWidget *parent = 0);
    ~ecoinconfirmcreatedialog();



    void settrainfo(QStringList data);

    void processSendCoinsReturn_(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());

    void setModel(WalletModel *_model);

    std::string m_sendcoinerror;

Q_SIGNALS:
    void CreateeCoinSuccess();
    void backtoecoincreate();

private Q_SLOTS:
    void on_pushButton_createeCoin_pressed();

    void on_pushButton_pressed();

private:
    Ui::ecoinconfirmcreatedialog *ui;


    WalletModel *walletModel;
   // WalletModelTransaction *m_tra;



};

#endif // ECOINCONFIRMCREATEDIALOG_H
