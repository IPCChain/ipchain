#ifndef SENDCOINSAFFRIMWIDGET_H
#define SENDCOINSAFFRIMWIDGET_H

#include <QWidget>
#include "walletmodel.h"
#include <QDialog>
#include <QMessageBox>
#include <QString>
#include <QTimer>

class WalletModel;
class ClientModel;
class OptionsModel;
class PlatformStyle;
class SendCoinsRecipient;


namespace Ui {
class SendCoinsAffrimWidget;
}
QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE
class SendCoinsAffrimWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SendCoinsAffrimWidget(bool islocked,QWidget *parent = 0);
    ~SendCoinsAffrimWidget();
    void setMessage(QString a,QString b,QString label,int c);
    void setModel(WalletModel *_model);
    void setClientModel(ClientModel *clientModel);
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());

Q_SIGNALS:
    void message(const QString &title, const QString &message, unsigned int style);
    void goresultpage();
    void gobacktosendpage();
private Q_SLOTS:
    void on_sendButton_pressed();
    void on_pushButton_pressed();

private:
    Ui::SendCoinsAffrimWidget *ui;
    WalletModel *model;
    ClientModel *clientModel;
    bool fNewRecipientAllowed;
    bool m_isLocked;
    int eTag;
    QString eLabel;

};



class SendConfirmationDialog : public QMessageBox
{
    Q_OBJECT

public:
    SendConfirmationDialog(const QString &title, const QString &text, int secDelay = 0, QWidget *parent = 0);
    int exec();


private Q_SLOTS:
    void countDown();
    void updateYesButton();

private:
    QAbstractButton *yesButton;
    QTimer countDownTimer;
    int secDelay;

};

#endif // SENDCOINSAFFRIMWIDGET_H
