#ifndef SETMESSAGEAUTHENTICATION_H
#define SETMESSAGEAUTHENTICATION_H

#include <QWidget>
class SetMessageAuthenticationTab;
class SetMessageSignature;
class WalletModel;
namespace Ui {
class SetMessageAuthentication;
}

class SetMessageAuthentication : public QWidget
{
    Q_OBJECT

public:
    explicit SetMessageAuthentication(QWidget *parent = 0);
    ~SetMessageAuthentication();
    WalletModel *walletModel;
    void setModel(WalletModel * model);
private Q_SLOTS:
    void on_pushButton_msgsignature_pressed();
    void on_pushButton__msgauthentication_pressed();
    void on_pushButton_back_pressed();
Q_SIGNALS:
    void back();
private:
    Ui::SetMessageAuthentication *ui;
    SetMessageAuthenticationTab *SetMessageAuthenticationTabPage;
    SetMessageSignature *SetMessageSignaturePage;
};

#endif // SETMESSAGEAUTHENTICATION_H
