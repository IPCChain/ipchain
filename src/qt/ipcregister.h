#ifndef IPCREGISTER_H
#define IPCREGISTER_H

#include <string>
using namespace std;
#include <QWidget>
class WalletModel;
namespace Ui {
class IpcRegister;
}

class IpcRegister : public QWidget
{
    Q_OBJECT

public:
    explicit IpcRegister(QWidget *parent = 0);
    ~IpcRegister();
    void setaddress(QString);
    void fileToMd5(std::string strmd5);
    WalletModel *walletModel;
    void setModel(WalletModel * model){walletModel = model;}
    void showEvent(QShowEvent *);
    void clearData();
    QString m_address;
public Q_SLOTS:
    void on_pushButton_next_pressed();
    void on_pushButton_back_pressed();
    void on_pushButton_browse_pressed();
    void showmd5Slot(QString);
    void on_pushButton_selectaddress_pressed();
    void slotTextChanged();
Q_SIGNALS:
    void back();
    void next(QStringList);
    void showmd5(QString);
    void gotoIpcSelectAddressPage();


private:
    Ui::IpcRegister *ui;
};



#endif // IPCREGISTER_H
