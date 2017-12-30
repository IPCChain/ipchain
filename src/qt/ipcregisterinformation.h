#ifndef IPCREGISTERINFORMATION_H
#define IPCREGISTERINFORMATION_H

#include <QWidget>
class WalletModel;
namespace Ui {
class IpcRegisterInformation;
}

class IpcRegisterInformation : public QWidget
{
    Q_OBJECT

public:
    explicit IpcRegisterInformation(QWidget *parent = 0);
    ~IpcRegisterInformation();
    WalletModel *walletModel;
    void setModel(WalletModel * model){walletModel = model;}
    void setinfos(QStringList infos);
private Q_SLOTS:
    void on_pushButton_ok_pressed();
    void on_pushButton_back_pressed();
Q_SIGNALS:
    void back();
    void next(int);
private:
    Ui::IpcRegisterInformation *ui;
};

#endif // IPCREGISTERINFORMATION_H
