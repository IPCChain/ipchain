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
    void resetinfo(int index);
    QString getStartTime();
    int m_nTimerId;
    void timerEvent( QTimerEvent *event );

private Q_SLOTS:
    void on_pushButton_back_pressed();
    void on_pushButton_transfer_pressed();
    void on_pushButton_authorization_pressed();
    void on_pushButton_toimage_pressed();

Q_SIGNALS:
    void back();
    void gotoIpcTransferTransactionPage(QString s1);
    void gotoIpcAuthorizationTransactionPage(QString s1);
    void ShowLabel_t9();
private:
    Ui::IpcDetails *ui;
    QString m_strExclusive;
    int m_index;

    bool m_pushButton_transfer_show;
    bool m_pushButton_authorization_show;

    QString m_ui_label_t5;
    QString m_ui_label_t6;
    QString QChineseTimeToEnglish(QString);
    bool canTransaction();
    bool m_bCanTransaction;
};

#endif // IPCDETAILS_H
