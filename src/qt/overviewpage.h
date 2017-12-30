// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_OVERVIEWPAGE_H
#define BITCOIN_QT_OVERVIEWPAGE_H
//QTableView de tableview
#include "amount.h"
#include "guiutil.h"//20170807
#include <QWidget>
#include <memory>

class ClientModel;
class TransactionFilterProxy;
class TxViewDelegate;
class PlatformStyle;
class WalletModel;
class TransactionView;
class TransactionRecord;
class InfoWidget;
class sendhistory;
class RecvHistory;
class sendipchistory;
class recvipchistory;
class SendTokenHistory;
class RecvTokenHistory;
namespace Ui {
class overviewpage;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Overview ("home") page widget */
class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~OverviewPage();
    //static OverviewPagem;


    static quint32 TYPE(int type) { return 1<<type; }

    enum WatchOnlyFilter
    {
        WatchOnlyFilter_All,
        WatchOnlyFilter_Yes,
        WatchOnlyFilter_No
    };


    enum DateEnum
    {
        All,
        Today,
        ThisWeek,
        ThisMonth,
        LastMonth,
        ThisYear,
        Range
    };

    void addinfo(InfoWidget *infopage);


    void addsendpage(sendhistory *sendpage);
    void addrecv(RecvHistory *recvpage);
    void addsendipcpage(sendipchistory *sendipcpage);
    void addrecvipcpage(recvipchistory *recvipcpage);
    void addsendtokenpage(SendTokenHistory *sendTokenPage);
    void addrecvtokenpage(RecvTokenHistory *recvTokenPage);


    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
  //  void showOutOfSyncWarning(bool fShow);
   // void setNumBlocks(int count, const QDateTime& blockDate, double nVerificationProgress, bool header);
    GUIUtil::TableViewLastColumnResizingFixer *columnResizingFixer;//20170807 kingofworld
public Q_SLOTS:
    void setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance,
                    const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance, const CAmount& depositBalance);
    void showDetails(QModelIndexList index);

Q_SIGNALS:
    void transactionClicked(const QModelIndex &index);
    void transactionnnClicked(const QModelIndex &index,quint8 t);
    void outOfSyncWarningClicked();
    void openTransactionDescPage(QModelIndexList);
    void chaininfoClicked();
    void selectLabel(const QModelIndex &index);
private:
    Ui::overviewpage *ui;
    int updatetime = 0;
    QMenu *contextMenu;
    ClientModel *clientModel;
    WalletModel *walletModel;
    CAmount currentBalance;
    CAmount currentUnconfirmedBalance;
    CAmount currentImmatureBalance;
    CAmount currentWatchOnlyBalance;
    CAmount currentWatchUnconfBalance;
    CAmount currentWatchImmatureBalance;
    CAmount currentdepositBalance;

    int typerole;

    TxViewDelegate *txdelegate;
    std::unique_ptr<TransactionFilterProxy> filter;

    std::vector<QWidget*> vWidget;
    std::vector<QWidget*>::iterator arr;
    QWidget* wWidget;
    InfoWidget * singleton1;
    QModelIndex index_;

private Q_SLOTS:
    void chooseDate(int idx);
    void updateDisplayUnit();
    void handleTransactionClicked(const QModelIndex &index);

    void handleOutOfSyncWarningClicks();
  //  void on_comboBox_activated(int index);
    void on_listTransactions_clicked(const QModelIndex &index);
    void on_showdetailButton_pressed();
  //  void updateinfo(int count, const QDateTime& blockDate, double nVerificationProgress);

};

#endif // BITCOIN_QT_OVERVIEWPAGE_H
