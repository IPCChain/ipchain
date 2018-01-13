#ifndef WALLETPASSWORD_H
#define WALLETPASSWORD_H

#include <QDialog>

namespace Ui {
class WalletPassword;
}

class WalletPassword : public QDialog
{
    Q_OBJECT

public:
    explicit WalletPassword(QWidget *parent = 0);
    ~WalletPassword();
    void setDlgParams();
    QString GetPassword(){return m_password;}


private Q_SLOTS:
    void on_pushButton_ok_pressed();

private:
    Ui::WalletPassword *ui;
    QString m_password;
};

#endif // WALLETPASSWORD_H
