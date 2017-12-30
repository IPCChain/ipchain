#ifndef IPCDIALOG_H
#define IPCDIALOG_H

#include <QWidget>
#include <QVBoxLayout>
#include <QVector>
#include <string>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>

using namespace std;
class WalletModel;
class IpcDetails;
class IpcRegister;
class IpcRegisterInformation;
class SuccessfulTrade;
class IpcInspectionTag;
class IpcTransferTransaction;
class IpcAuthorizationTransaction;
class ipcSelectAddress;
namespace Ui {
class ipcdialog;
}

class ipcdialog : public QWidget
{
    Q_OBJECT

public:
    explicit ipcdialog(QWidget *parent = 0);
    ~ipcdialog();
    WalletModel *walletModel;
    void setModel(WalletModel * model);//{walletModel = model;}
    int addnewoneipcmessage(int pictype,QString name,QString time,int picprogess,int dlgnum,QString txid);
    void showEvent(QShowEvent *);
    static void updatalist();

private:
    Ui::ipcdialog *ui;
    QVBoxLayout * pvboxlayoutall;   
    QVector<QStringList> m_ipclist;
    QStackedWidget *walletStackBranchPage;
    IpcDetails*  ipcdetailsPage;
    IpcRegister*  IpcRegisterPage;
    IpcRegisterInformation*  IpcRegisterInformationPage;
    SuccessfulTrade*  SuccessfulTradePage;
    IpcInspectionTag*  IpcInspectionTagPage;
    IpcTransferTransaction*  IpcTransferTransactionPage;
    IpcAuthorizationTransaction*  IpcAuthorizationTransactionPage;
    ipcSelectAddress*  ipcSelectAddressPage;
    QLabel* pOldSelectIpcButtons;

Q_SIGNALS:
    //void openipcdetailsdialog(QString);
    void openRegisterPage();
    void openInspectionTagPage();
private Q_SLOTS:
    void on_pushButton_register_pressed();
    void on_pushButton_label_pressed();
    void gotoIpcdetailsPage(QString txid = "");
    void gotoIpcdetailsPage(int index);
    void gotoBackIpcRegisterPage();
    void gotoIpcRegisterPage(QString address = "");
    void gotoIpcRegisterInformationPage(QStringList);
    void gotoSuccessfulTradePage(int type);
    void gotoIpcInspectionTagPage();
    void gotoIpcSelectAddressPage();
    void gotoIpcTransferTransactionPage(QString s1);
    void gotoIpcAuthorizationTransactionPage(QString s1);
    void changelabel(QLabel*);
    void updateIpcList();
};

#endif // IPCDIALOG_H
