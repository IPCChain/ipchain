#ifndef PASSWORDSETTINGWIDGET_H
#define PASSWORDSETTINGWIDGET_H

#include <QWidget>
#include <QDialog>

class WalletModel;
namespace Ui {
class PasswordSettingWidget;
}

class PasswordSettingWidget : public QWidget
{
    Q_OBJECT

public:
    enum Mode {
        Encrypt,    /**< Ask passphrase twice and encrypt */
        Unlock,     /**< Ask passphrase and unlock */
        ChangePass, /**< Ask old passphrase + new passphrase twice */
        Decrypt     /**< Ask passphrase and decrypt wallet */
    };


    explicit PasswordSettingWidget(Mode _mode,QWidget *parent = 0);
    ~PasswordSettingWidget();

    void setMode(Mode _mode);
    void setModel(WalletModel *model);


    int getTag();
    void setTag(int tag);
Q_SIGNALS:
        void ChangePasswordSuccess();
        void openSendCoinsAffrimwidget();
        void back();
private Q_SLOTS:
    void on_pushButton_pressed();
    void on_pushButton_2_pressed();
    void textChanged();
    void secureClearPassFields();
private:
    Ui::PasswordSettingWidget *ui;
    Mode mode;
    WalletModel *model;
    bool fCapsLock;
    int m_tag;
protected:
    bool event(QEvent *event);
};

#endif // PASSWORDSETTINGWIDGET_H
