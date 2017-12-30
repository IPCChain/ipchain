#ifndef WALLETPAGEBUTTONS_H
#define WALLETPAGEBUTTONS_H

#include <QWidget>
#include <QPushButton>
namespace Ui {
class walletpagebuttons;
}

class walletpagebuttons : public QWidget
{
    Q_OBJECT

public:
    explicit walletpagebuttons(QWidget *parent = 0);
    ~walletpagebuttons();

    void fresh(int concount,int count);

private Q_SLOTS:
    void on_pushButton_set_pressed();
    void on_pushButton_setpic_pressed();
    void on_pushButton_ipc_pressed();
    void on_pushButton_ipcpic_pressed();
    void on_pushButton_recive_pressed();
    void on_pushButton_recivepic_pressed();
    void on_pushButton_send_pressed();
    void on_pushButton_sendpic_pressed();
    void on_pushButton_wallet_pressed();
    void on_pushButton_walletpic_pressed();
    void getWalletPageButtonsStatus();
    void on_pushButton_markbill_pressed();

    void on_pushButton_eipc_pressed();

Q_SIGNALS:
    void setpressed();
    void ipcpressed();
    void recivepressed();
    void sendpressed();
    void walletpressed();
    void tallypressed();
    void eipcpressed();
private:
    Ui::walletpagebuttons *ui;
    void setpushbuttonchecked(QString name,QPushButton* wallet,\
      QString graypicname,QString greenpicname);


    QString formatLang() ;
};

#endif // WALLETPAGEBUTTONS_H
