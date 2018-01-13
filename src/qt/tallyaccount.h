#ifndef TALLYACCOUNT_H
#define TALLYACCOUNT_H

#include <QWidget>
#include "walletmodel.h"
#include "upgradewidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
class WalletModel;
namespace Ui {
class TallyAccount;
}

class TallyAccount : public QWidget
{
    Q_OBJECT

public:
    explicit TallyAccount(QWidget *parent = 0);
    ~TallyAccount();
    void setinfo(WalletModel::keepupaccountInfo info);
    WalletModel::keepupaccountInfo getinfo();
    void setModel(WalletModel *_model);
    void  setfinishinfo();
    void resettime();
    void  GetTotalAmount();
    void getPasswordIfTallyAccountIng();
    void  showPwdErrAndStop();
private Q_SLOTS:
    void on_pushButton_outaccount_pressed();
    void updateIpcList();

private:
    Ui::TallyAccount *ui;
    WalletModel *walletmodel;
    WalletModel::keepupaccountInfo info_;
    std::vector<WalletModel::CBookKeep> m_bookkeeplist;
    QVBoxLayout  *pvboxlayoutall;
    void addItemContent(int row, int column, QString content);
    int m_time;
Q_SIGNALS:
    void next(QString add,CAmount num);
protected:
   void timerEvent( QTimerEvent *event );
   int m_nTimerId;


};

#endif // TALLYACCOUNT_H
