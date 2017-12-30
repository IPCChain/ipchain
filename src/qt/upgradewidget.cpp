#include "upgradewidget.h"
#include "log/log.h"

#include <QMessageBox>
#include <QMouseEvent>
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
    if(2== getIntertag())//send dialog
    {

        Q_EMIT pressw(this->m_add,this->m_label);
    }
    else if(4== getIntertag())//ecoin dialog
    {
        Q_EMIT pressecoin(this->eMsg_);
    }

    else if(5 == getIntertag())//set dialog
    {
        Q_EMIT presswq(this->m_add);
    }
    else if(3 == getIntertag())//ecoin address dialog
    {
    }
    else
    {
        Q_EMIT pressed();
    }
}
void upgradewidget::setMessage(eCoinMsg  eMsg)
{this->eMsg_=eMsg;}
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
    LOG_WRITE(LOG_INFO,"sendemit ",QString::number(m_dialognum).toStdString().c_str());
    printf("sendemit\n");
    Q_EMIT dlgnum(m_dialognum);
    Q_EMIT dlgtxid(m_txid);
}

void upgradewidget::changetext(QString tx)
{

    Q_EMIT  updateinfo(tx,this->m_add);
}
