// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_SENDCOINSDIALOG_H
#define BITCOIN_QT_SENDCOINSDIALOG_H

#include "walletmodel.h"

#include <QDialog>
#include <QMessageBox>
#include <QString>
#include <QTimer>

class ClientModel;
class OptionsModel;
class PlatformStyle;
class SendCoinsEntry;
class SendCoinsRecipient;

namespace Ui {
class SendCoinsDialog;
}

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

/** Dialog for sending bitcoins */
class SendCoinsDialog : public QDialog
{
    Q_OBJECT
public:
    enum coinper
    {
        IPC,
        mIPC,
        uIPC,
        zhi
    };
    explicit SendCoinsDialog(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~SendCoinsDialog();

    void setClientModel(ClientModel *clientModel);
    void setModel(WalletModel *model);

    QWidget *setupTabChain(QWidget *prev);

    void setAddress(const QString &address,const QString &label);
    void clearInfo();
public Q_SLOTS:
    void setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance,
                    const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance);

private:
    Ui::SendCoinsDialog *ui;
    ClientModel *clientModel;
    WalletModel *model;
    bool fNewRecipientAllowed;
    bool fFeeMinimized;
    const PlatformStyle *platformStyle;
    int eTag = 0;

    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());
    void minimizeFeeSection(bool fMinimize);
    void updateFeeMinimizedLabel();


private Q_SLOTS:
    void on_AddAdsButton_pressed();
    void on_scanBtn_pressed();
    void on_GoSettingBtn_pressed();
    void coinUpdate(int idx);

Q_SIGNALS:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
    void openSettingwidget(QString a,QString b,QString label,int tag);
    void openSendAffrimwidget(QString a,QString b,QString label,int tag);
    void  openAddBookPagewidget(AddressTableModel *atm,int tag);
};

#endif // BITCOIN_QT_SENDCOINSDIALOG_H
