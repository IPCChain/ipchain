#ifndef SETDIALOG_H
#define SETDIALOG_H

#include <QWidget>
#include "walletmodel.h"
#include <QStackedWidget>
#include <QPushButton>
class exportdialog;
class SetRecovery;
class SetMessageAuthenticationTab;
class SetMessageSignature;
class SuccessfulTrade;
class PasswordSettingWidget;
class AddBookWidget;
namespace Ui {
class setdialog;
}

class setdialog : public QWidget
{
    Q_OBJECT

public:
    explicit setdialog(QWidget *parent = 0);
    ~setdialog();

    void setModel(WalletModel *_model);
    void setPsdSetSuccess();
    void setPsdshow(PasswordSettingWidget  *PasswordSettingPage);
    void setAddpageshow(AddBookWidget *Addbookpage);
    void setButtonPic(QPushButton*);
    void gotoSetMessageAuthenticationTabPage();
    void gotoSetMessageSignaturePage();
private Q_SLOTS:
    void on_pushButton_export_pressed();
    void on_pushButton_recovery_pressed();
    void on_pushButton_msgauthentication_pressed();
    void on_pushButton_addressbook_pressed();
    void on_pushButton_setpwd_pressed();
    void gotoExportPageSlot();
    void gotoSetRecoveryPageSlot();
    void gotoMessageAuthenticationPageSlot();
    void gotoSuccessfulTradePageSlot(int type);
    void on_pushButton_aboutwallet_pressed();
    void on_pushButton_verification_pressed();

Q_SIGNALS:
    void openexportdialog();
    void gotoSetRecoveryPage();
    void gotoMessageAuthenticationPage();
    void openAddBookPagewidget(AddressTableModel *atm,int tag);
    void openPasswordSetwidget(int tag);

private:
    Ui::setdialog *ui;
    WalletModel *model;
    exportdialog* exportdialogPage;
    SetRecovery* SetRecoveryPage;
    QStackedWidget *walletStackBranchPage;
    SuccessfulTrade*  SuccessfulTradePage;
    SetMessageAuthenticationTab *SetMessageAuthenticationTabPage;
    SetMessageSignature *SetMessageSignaturePage;

};

#endif // SETDIALOG_H
