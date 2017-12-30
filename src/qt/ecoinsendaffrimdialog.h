#ifndef ECOINSENDAFFRIMDIALOG_H
#define ECOINSENDAFFRIMDIALOG_H

#include <QWidget>

#include "walletmodel.h"
#include "upgradewidget.h"

class WalletModel;


namespace Ui {
class ecoinsendaffrimdialog;
}

class ecoinsendaffrimdialog : public QWidget
{
    Q_OBJECT

public:
    explicit ecoinsendaffrimdialog(QWidget *parent = 0);
    ~ecoinsendaffrimdialog();
    void setMsg(QString,QString);
    void setModel(WalletModel *_model);
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());
Q_SIGNALS:
    void SendeCoinSuccess();
private Q_SLOTS:
    void on_sendecoinButton_pressed();

private:
    Ui::ecoinsendaffrimdialog *ui;
    WalletModelTransaction *m_currentTransaction;
    WalletModel *model;
    bool fNewRecipientAllowed;
    bool m_isLocked;
    int eTag;
    QString m_name;
    QString m_num;
    std::string m_error;
};

#endif // ECOINSENDAFFRIMDIALOG_H
