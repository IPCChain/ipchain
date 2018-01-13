#ifndef IPCINSPECTIONTAG_H
#define IPCINSPECTIONTAG_H
#include <string>
using namespace std;
#include <QWidget>
class WalletModel;
namespace Ui {
class IpcInspectionTag;
}
class IpcInspectionTag : public QWidget
{
    Q_OBJECT

public:
    explicit IpcInspectionTag(QWidget *parent = 0);
    ~IpcInspectionTag();
    WalletModel *walletModel;
    void setModel(WalletModel * model){walletModel = model;}
    void fileToMd5(string strmd5);
    void cleardata();

private Q_SLOTS:
    void on_pushButton_back_pressed();
    void on_pushButton_select_pressed();
    void showmd5Slot(QString strmd5);
    void on_pushButton_inspectiontag_pressed();

Q_SIGNALS:
    void back();
    void showmd5(QString);
private:
    Ui::IpcInspectionTag *ui;
};

#endif // IPCINSPECTIONTAG_H
