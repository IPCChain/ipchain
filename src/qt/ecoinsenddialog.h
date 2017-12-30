#ifndef ECOINSENDDIALOG_H
#define ECOINSENDDIALOG_H

#include <QWidget>
#include "upgradewidget.h"

#include "guiutil.h"

#include <QImage>
#include <QPainter>
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
Q_SIGNALS:
    void gotoSendAffrimDialog(QString name,QString num);
private Q_SLOTS:
    void on_GoBtn_pressed();

private:
    Ui::eCoinSenddialog *ui;
    QString m_name;
    QString m_num;
    QString m_address;

};

#endif // ECOINSENDDIALOG_H
