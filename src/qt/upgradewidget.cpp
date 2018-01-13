#include "upgradewidget.h"
#include "log/log.h"
#include <QMouseEvent>
#include <QMessageBox>
upgradewidget::upgradewidget(QWidget *parent) : QWidget(parent),pipcselectlabel(NULL)
{
    m_dialognum = 0;
    connect(this,SIGNAL(pressed()),this,SLOT(myPressedDown()));
}

void upgradewidget::mousePressEvent(QMouseEvent *e)
{
    QMouseEvent *ev = (QMouseEvent*)e;
    if(ev->button() == Qt::RightButton)
    {
        return ;
    }
    if(TAB_two== getIntertag())
    {
        Q_EMIT pressw(this->m_add,this->m_label);
    }
    else if(TAB_four== getIntertag())
    {
        Q_EMIT pressecoin(this->eMsg_);
    }

    else if(TAB_three == getIntertag() || TAB_five == getIntertag())
    {
    }
    else
    {
        Q_EMIT pressed();
    }
}
void upgradewidget::setMessage(eCoinMsg  eMsg)
{
    this->eMsg_=eMsg;
}
void upgradewidget::setadd(QString add)
{
    this->m_add = add;
}
void upgradewidget::setlabel(QString label)
{
    this->m_label = label;
}
void upgradewidget::setIntertag(int tag)
{
    this->eInterTag = tag;
}
int upgradewidget::getIntertag()
{
    return this->eInterTag;
}
void upgradewidget::myPressedDown()
{
    if(pipcselectlabel){
        pipcselectlabel->show();
        Q_EMIT selectlabel(pipcselectlabel);
    }

    Q_EMIT dlgnum(m_dialognum);
    Q_EMIT dlgtxid(m_txid);
}

void upgradewidget::changetext(QString tx)
{
    Q_EMIT  updateinfo(tx,this->m_add);
}
void upgradewidget::focusInEvent(QFocusEvent *e)
{
    QPalette p=QPalette();
    p.setColor(QPalette::Base,Qt::black);
    setPalette(p);
}

void upgradewidget::focusOutEvent(QFocusEvent *e)
{
    this->clearFocus();
    QPalette p1=QPalette();
    p1.setColor(QPalette::Base,Qt::white);
    setPalette(p1);
    setCursor(Qt::BlankCursor);
}
