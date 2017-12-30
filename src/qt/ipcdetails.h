#ifndef IPCDETAILS_H
#define IPCDETAILS_H

#include <QWidget>
class WalletModel;
namespace Ui {
class IpcDetails;
}

class IpcDetails : public QWidget
{
    Q_OBJECT

public:
    explicit IpcDetails(QWidget *parent = 0);
    ~IpcDetails();
    WalletModel *walletModel;
    void setWalletModel(WalletModel * model){walletModel = model;}
    //void resetinfo(QString txid);
    void resetinfo(int index);
    QString getStartTime();

private Q_SLOTS:
    void on_pushButton_back_pressed();
    void on_pushButton_transfer_pressed();

    void on_pushButton_authorization_pressed();

    void on_pushButton_toimage_pressed();

Q_SIGNALS:
    void back();
    void gotoIpcTransferTransactionPage(QString s1);
    void gotoIpcAuthorizationTransactionPage(QString s1);
private:
    Ui::IpcDetails *ui;
    QString m_strExclusive;
};

#endif // IPCDETAILS_H
