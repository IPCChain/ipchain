#ifndef TALLYOUTACCOUNT_H
#define TALLYOUTACCOUNT_H

#include <QWidget>
#include <QMessageBox>
#include "walletmodel.h"

class WalletModel;
namespace Ui {
class TallyOutAccount;
}

class TallyOutAccount : public QWidget
{
    Q_OBJECT

public:
    explicit TallyOutAccount(QWidget *parent = 0);
    ~TallyOutAccount();

    void setModel(WalletModel *_model);
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());
    void setinfo(WalletModel::keepupaccountInfo info);
    WalletModel::keepupaccountInfo getinfo();
    void clearinfo();
    std::string m_error;

private Q_SLOTS:
    void on_pushButton_Cancle_pressed();

    void on_pushButton_OK_pressed();
private:
    Ui::TallyOutAccount *ui;
    WalletModel *walletmodel;

    bool fNewRecipientAllowed;
    bool m_isLocked;
    int eTag;


    WalletModel::keepupaccountInfo info_;

    bool setlabel_errmsg();
Q_SIGNALS:
    void next(bool);
};

#endif // TALLYOUTACCOUNT_H
