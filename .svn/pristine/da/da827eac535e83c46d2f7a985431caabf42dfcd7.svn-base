#include "walletpagebuttons.h"
#include "forms/ui_walletpagebuttons.h"
#include <QPalette>
#include <QSettings>
#include "util.h"
#include <QMessageBox>
#include "intro.h"
#include "cmessagebox.h"
#include "ipchaingui.h"
#include "log/log.h"
#include <QDesktopWidget>
extern bool isfirstfullloaded ;
extern Gerxy Gerxy_;
extern int titleBarHeight;
QString walletpagebuttons::formatLang()
{
    QSettings settings;
    // Get desired locale (e.g. "de_DE")
    // 1) System default language
    QString lang_territory = QLocale::system().name();
    // 2) Language from QSettings
    QString lang_territory_qsettings = settings.value("language", "").toString();
    if(!lang_territory_qsettings.isEmpty())
        lang_territory = lang_territory_qsettings;
    // 3) -lang command line argument
    lang_territory = QString::fromStdString(GetArg("-lang", lang_territory.toStdString()));
    // return lang_territory;



    QLocale locale;
    if( locale.language() == QLocale::English )
    {

        return "English";
    }
    else if( locale.language() == QLocale::Chinese )
    {
        return "Chinese";
    }
}
walletpagebuttons::walletpagebuttons(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::walletpagebuttons)
{
    ui->setupUi(this);

    QString m_lang =formatLang();

    if(Intro::m_clienttype == "test")
    {
        if("Chinese"==m_lang)
        {
            ui->label->setPixmap(QPixmap(":/res/png/titlelogo_d.png"));
        }
        else
        {
            ui->label->setPixmap(QPixmap(":/res/png/titlelogo_en.png"));
        }
        ui->label->show();
    }
    else
    {
        if("Chinese"==m_lang)
        {
            ui->label->setPixmap(QPixmap(":/res/png/titlelogo.png"));
        }
        else
        {
            ui->label->setPixmap(QPixmap(":/res/png/titlelogo_nen.png"));
        }

        ui->label->show();
    }
    ui->pushButton_wallet->setChecked(true);
    connect(ui->pushButton_ipc,SIGNAL(pressed()),this,SLOT(getWalletPageButtonsStatus()));
    connect(ui->pushButton_recive,SIGNAL(pressed()),this,SLOT(getWalletPageButtonsStatus()));
    connect(ui->pushButton_send,SIGNAL(pressed()),this,SLOT(getWalletPageButtonsStatus()));
    connect(ui->pushButton_set,SIGNAL(pressed()),this,SLOT(getWalletPageButtonsStatus()));
    connect(ui->pushButton_wallet,SIGNAL(pressed()),this,SLOT(getWalletPageButtonsStatus()));
    connect(ui->pushButton_markbill,SIGNAL(pressed()),this,SLOT(getWalletPageButtonsStatus()));

    connect(ui->pushButton_eipc,SIGNAL(pressed()),this,SLOT(getWalletPageButtonsStatus()));
    connect(ui->pushButton_ipc,SIGNAL(pressed()),this,SLOT(getWalletPageButtonsStatus()));



    QString button_style=
            "QPushButton{image:url(:/res/png/wallet.png);color: white; border: none}"
            "QPushButton:pressed{image:url(:/res/png/wallet_d.png);color:black; }";
    ui->pushButton_wallet->setStyleSheet(button_style);


    ui->pushButton_send->setStyleSheet("background-image:url(:/res/png/send.png);color: white;border:none;font-size:16px; ");
    ui->pushButton_recive->setStyleSheet("background-image:url(:/res/png/recv.png);color: white;border:none;font-size:16px");
    ui->pushButton_ipc->setStyleSheet("background-image:url(:/res/png/ipc.png);color: white;border:none;font-size:16px;");
    ui->pushButton_eipc->setStyleSheet("background-image:url(:/res/png/eIPC.png);color: white;border:none;font-size:16px;");
    ui->pushButton_markbill->setStyleSheet("background-image:url(:/res/png/markbill.png);color: white;border:none;font-size:16px;");
    ui->pushButton_set->setStyleSheet("background-image:url(:/res/png/setting.png);color: white;border:none;font-size:16px;");

    setpushbuttonchecked("pushButton_wallet",ui->pushButton_wallet,"wallet.png","wallet_d.png");

    ui->pushButton_wallet->setText(tr("\n\n\n")+tr("wallet"));

    ui->pushButton_send->setText(tr("\n\n\n")+tr("send"));
    ui->pushButton_recive->setText(tr("\n\n\n")+tr("receive"));
    ui->pushButton_ipc->setText(tr("\n\n\n")+tr("IP"));
    ui->pushButton_eipc->setText(tr("\n\n\n")+tr("token"));
    ui->pushButton_markbill->setText(tr("\n\n\n")+tr("markbill"));
    ui->pushButton_set->setText(tr("\n\n\n")+tr("set"));
}

walletpagebuttons::~walletpagebuttons()
{
    delete ui;
}

void walletpagebuttons::on_pushButton_walletpic_pressed()
{


    if(isfirstfullloaded)
    {
        Q_EMIT walletpressed();
    }
    else
    {
        CMessageBox msg;
        setMsgDlgPlace(&msg);

       // msg.setGeometry(Gerxy_.x_+(1000-524)/2,
       // Gerxy_.y_+(660+titleBarHeight-287)/2,524,287);

        msg.setIsClose(false);
        msg.setCacelVisible(false);
        msg.setMessage(3);
        msg.exec();

    }
}

void walletpagebuttons::fresh(int concount,int count)
{
    ui->nodenum->setText(QString::number(concount));
    ui->heightnum->setText(QString::number(count));
}

void walletpagebuttons::on_pushButton_wallet_pressed()
{

    Q_EMIT walletpressed();
}

void walletpagebuttons::on_pushButton_sendpic_pressed()
{
    if(isfirstfullloaded)
    {

        Q_EMIT sendpressed();
    }
    else
    {
        CMessageBox msg;
        setMsgDlgPlace(&msg);
        msg.setIsClose(false);
        msg.setCacelVisible(false);
        msg.setMessage(3);
        msg.exec();
    }
}

void walletpagebuttons::on_pushButton_send_pressed()
{


    if(isfirstfullloaded)
    {
        Q_EMIT sendpressed();
    }
    else
    {
        CMessageBox msg;
        setMsgDlgPlace(&msg);
        msg.setIsClose(false);
        msg.setCacelVisible(false);
        msg.setMessage(3);
        msg.exec();

    }
}

void walletpagebuttons::on_pushButton_recivepic_pressed()
{
    if(isfirstfullloaded)
    {

        Q_EMIT recivepressed();
    }
    else
    {
        CMessageBox msg;
        setMsgDlgPlace(&msg);
        msg.setIsClose(false);
        msg.setCacelVisible(false);
        msg.setMessage(3);
        msg.exec();

    }
}

void walletpagebuttons::on_pushButton_recive_pressed()
{
    if(isfirstfullloaded)
    {

        Q_EMIT recivepressed();
    }
    else
    {
        CMessageBox msg;
        setMsgDlgPlace(&msg);
        msg.setIsClose(false);
        msg.setCacelVisible(false);
        msg.setMessage(3);
        msg.exec();
        ui->pushButton_wallet->setChecked(true);

    }
}

void walletpagebuttons::on_pushButton_ipcpic_pressed()
{
    if(isfirstfullloaded)
    {

        Q_EMIT ipcpressed();
    }
    else
    {
        CMessageBox msg;
        setMsgDlgPlace(&msg);
        msg.setIsClose(false);
        msg.setCacelVisible(false);
        msg.setMessage(3);
        msg.exec();
        ui->pushButton_wallet->setChecked(true);

    }
}

void walletpagebuttons::on_pushButton_ipc_pressed()
{
    if(isfirstfullloaded)
    {

        Q_EMIT ipcpressed();
    }
    else
    {
        CMessageBox msg;
        setMsgDlgPlace(&msg);
        msg.setIsClose(false);
        msg.setCacelVisible(false);
        msg.setMessage(3);
        msg.exec();
        ui->pushButton_wallet->setChecked(true);

    }
}

void walletpagebuttons::on_pushButton_setpic_pressed()
{
    if(isfirstfullloaded)
    {

        Q_EMIT setpressed();
    }
    else
    {
        CMessageBox msg;
        setMsgDlgPlace(&msg);
        msg.setIsClose(false);
        msg.setCacelVisible(false);
        msg.setMessage(3);
        msg.exec();
        ui->pushButton_wallet->setChecked(true);

    }
}

void walletpagebuttons::on_pushButton_set_pressed()
{
    if(isfirstfullloaded)
    {

        Q_EMIT setpressed();
    }
    else
    {
        CMessageBox msg;
        setMsgDlgPlace(&msg);
        msg.setIsClose(false);
        msg.setCacelVisible(false);
        msg.setMessage(3);
        msg.exec();
        ui->pushButton_wallet->setChecked(true);
    }
}
void walletpagebuttons::getWalletPageButtonsStatus()
{
    QPushButton* button = qobject_cast<QPushButton*> (sender());
    QString name = button->objectName();

    setpushbuttonchecked(name,ui->pushButton_wallet,"wallet.png","wallet_d.png");
    if(isfirstfullloaded)
    {
        setpushbuttonchecked(name,ui->pushButton_send,"send.png","send_d.png");
        setpushbuttonchecked(name,ui->pushButton_recive,"recv.png","recv_d.png");
        setpushbuttonchecked(name,ui->pushButton_ipc,"ipc.png","ipc_d.png");
        setpushbuttonchecked(name,ui->pushButton_set,"setting.png","setting_d.png");
        setpushbuttonchecked(name,ui->pushButton_markbill,"markbill.png","markbill_d.png");

        setpushbuttonchecked(name,ui->pushButton_eipc,"eIPC.png","eIPC_d.png");
    }
    else
    {
        setpushbuttonchecked("pushButton_wallet",ui->pushButton_wallet,"wallet.png","wallet_d.png");
    }
}
void walletpagebuttons::setpushbuttonchecked(QString name,QPushButton* wallet,\
                                             QString graypicname,QString greenpicname)
{

    QString picbuttonstyle;
    picbuttonstyle = "QPushButton{border:none;background-image: \
            url(:/res/png/" ;
                QString wordbuttonstyle = "QPushButton{border:none;font-size:10px;color:";

            if(name == wallet->objectName())
    {
            picbuttonstyle.append( greenpicname + ");");
            picbuttonstyle.append("color: white;border:none;font-size:16px;};");
    }
            else
    {
            picbuttonstyle.append( graypicname + ");");
            picbuttonstyle.append("color: white;border:none;font-size:16px;};");
}
            wallet->setStyleSheet(picbuttonstyle);
}


void walletpagebuttons::on_pushButton_markbill_pressed()
{
    if(isfirstfullloaded)
    {
        Q_EMIT tallypressed();
    }
    else
    {
        CMessageBox msg;
        setMsgDlgPlace(&msg);
        msg.setIsClose(false);
        msg.setCacelVisible(false);
        msg.setMessage(3);
        msg.exec();
        ui->pushButton_wallet->setChecked(true);
    }
}

void walletpagebuttons::on_pushButton_eipc_pressed()
{
    if(isfirstfullloaded)
    {
        Q_EMIT eipcpressed();
    }
    else
    {
        CMessageBox msg;

        setMsgDlgPlace(&msg);
        msg.setIsClose(false);
        msg.setCacelVisible(false);
        msg.setMessage(3);
        msg.exec();
        ui->pushButton_wallet->setChecked(true);

    }
}
void walletpagebuttons::setMsgDlgPlace(CMessageBox* pmsgdlg)
{
    QDesktopWidget* desktopWidget = QApplication::desktop();
    QRect applicationRect = desktopWidget->screenGeometry();
    QRect rectgeometry = QApplication::desktop()->geometry();
    QPoint xy = mapToGlobal(mapToParent(mapToParent(this->pos())));
    pmsgdlg->move(applicationRect.x()+1000/2-524/2+xy.x(),applicationRect.y()+660/2-287/2+xy.y());


}
