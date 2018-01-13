#ifndef ECOINSENDDIALOG_H
#define ECOINSENDDIALOG_H

#include <QWidget>
#include "upgradewidget.h"
#include "guiutil.h"
#include <QImage>
#include <QPainter>

class WalletModel;

namespace Ui {
class eCoinSenddialog;
}

class eCoinSenddialog : public QWidget
{
    Q_OBJECT

public:
    explicit eCoinSenddialog(QWidget *parent = 0);
    ~eCoinSenddialog();
    void setMsg(QString name,QString num,QString address);
    void setModel(WalletModel *_model){m_model =_model;}
    void timerEvent( QTimerEvent *event );
Q_SIGNALS:
    void gotoSendAffrimDialog(QString name,QString num);
private Q_SLOTS:
    void on_GoBtn_pressed();
private:
    Ui::eCoinSenddialog *ui;
    QString m_name;
    QString m_num;
    QString m_address;
    int m_nTimerId;
    WalletModel *m_model;

};

#endif // ECOINSENDDIALOG_H
