#include "tallyaccount.h"
#include "forms/ui_tallyaccount.h"
#include "walletview.h"
#include "optionsmodel.h"
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QtCore>
#include "log/log.h"
#include <iostream>
#include <QTableWidgetItem>
#include <QLabel>
#include <QHeaderView>
#include "dpoc/DpocInfo.h"
#include <QDateTime>
#include <QSettings>
using namespace std;
#include "cmessagebox.h"
#include <QSettings>
#include <QPixmapCache>


class TextNet : public QObject
{
    Q_OBJECT//宏
public:
    static QString getHtml(QString url)
    {
        QNetworkAccessManager *manager = new QNetworkAccessManager();//实例化类
        QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(url)));//实例化类
        QByteArray responseData;//定义数据
        QEventLoop eventLoop;
        connect(manager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));//信号槽
        eventLoop.exec();       //block until finish
        responseData = reply->readAll();//读取返回数据
        delete manager;
        return QString(responseData);//返回数据
    }
};

TallyAccount::TallyAccount(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TallyAccount)
{
    ui->setupUi(this);
    ui->label_neterror->hide();
    m_nTimerId = startTimer(1000*60);

    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << tr("time")<<tr("money"));
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // ui->tableWidget->setRowCount(5);
#if QT_VERSION < 0x050000
    ui->tableWidget->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
#else
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
#endif
    m_time = 0;
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->label_punishment->setText("");
}

TallyAccount::~TallyAccount()
{
    killTimer(m_nTimerId);
    delete ui;
}
void TallyAccount::setinfo( WalletModel::keepupaccountInfo info)
{
    info_.Add_=info.Add_;
    info_.Coin_=info.Coin_;
    ui->label_punishment->setText("");
    ui->pushButton_outaccount->show();
    ui->label_neterror->hide();
    // timerEvent(NULL);
    QDateTime time = QDateTime::currentDateTime();
    m_time = time.toTime_t();
    ui->label_shownumtip->show();
    GetTotalAmount();
}
void TallyAccount::resettime()
{
    if(m_time > 0 )return;
    QDateTime time = QDateTime::currentDateTime();
    m_time = time.toTime_t();
}

void TallyAccount::setModel(WalletModel * model)
{
    walletmodel = model;
    if(walletmodel && walletmodel->getOptionsModel())
    {
        // connect(walletmodel->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayList(int)));
    }
    connect(walletmodel,SIGNAL(updateIpcList()),this,SLOT(updateIpcList()));
    updateIpcList();

    getPasswordIfTallyAccountIng();

}
WalletModel::keepupaccountInfo  TallyAccount::getinfo()
{
    return info_;
}

int TallyAccount::addnewoneipcmessage(QString name,QString time)//int pictype,,int picprogess,int dlgnum)
{
    upgradewidget * pnewone = new upgradewidget();
    //  pnewone->m_dialognum = dlgnum;
    pnewone->setMaximumHeight(70);
    pnewone->setMinimumHeight(70);
    //pnewone->setStyleSheet("QWidget{background-color:rgb(79,194,186)}");
    pnewone->setAutoFillBackground(true);
    QPalette palette;
    palette.setColor(QPalette::Background, QColor(79,194,186));
    //palette.setBrush(QPalette::Background, QBrush(QPixmap(":/background.png")));
    pnewone->setPalette(palette);

    QHBoxLayout * phboxlayout = new QHBoxLayout();
    phboxlayout->setContentsMargins(0,0,0,0);
    pnewone->setLayout(phboxlayout);

    static int num=0;
    num++;
    QLabel* pictypelabel = new QLabel(QString::number(num));
    // phboxlayout->addWidget(pictypelabel);

    QVBoxLayout * pvboxlayout = new QVBoxLayout();
    phboxlayout->addLayout(pvboxlayout);

    QLabel* namelabel = new QLabel(name);
    namelabel->setAttribute(Qt::WA_TranslucentBackground);
    QFont ftname;
    ftname.setPointSize(16);
    namelabel->setFont(ftname);
    QPalette paname;
    paname.setColor(QPalette::WindowText,Qt::white);
    namelabel->setPalette(paname);

    pvboxlayout->addWidget(namelabel);

    QLabel* timelabel = new QLabel(time);
    timelabel->setAttribute(Qt::WA_TranslucentBackground);
    QFont fttime;
    fttime.setPointSize(12);
    timelabel->setFont(fttime);
    QPalette patime;
    patime.setColor(QPalette::WindowText,Qt::white);
    timelabel->setPalette(patime);
    pvboxlayout->addWidget(timelabel);
    QSpacerItem* horizontalSpacer = new QSpacerItem(40,20,QSizePolicy::Expanding);
    phboxlayout->addSpacerItem(horizontalSpacer);
    QLabel* picendlabel = new QLabel();
    pnewone->setipcselectlabel(picendlabel);



    QPixmapCache::clear();

QPixmapCache::setCacheLimit(1);
QPixmap p;

p.load(":/res/png/triangle.png");
picendlabel->setPixmap(p);

 //   picendlabel->setPixmap(QPixmap(":/res/png/triangle.png"));
    // phboxlayout->addWidget(picendlabel);
    picendlabel->hide();
    pvboxlayoutall->addWidget(pnewone);

    return 1;

}

void TallyAccount::showEvent(QShowEvent *)
{

}

void TallyAccount::on_pushButton_outaccount_pressed()

{

    WalletModel::keepupaccountInfo accinfo = getinfo();

    Q_EMIT next(accinfo.Add_,accinfo.Coin_);
}

void TallyAccount::timerEvent( QTimerEvent *event )
{

    if(!CDpocInfo::Instance().IsHasLocalAccount())
    {
        return;
    }

    QString data = TextNet::getHtml(QString("http://www.baidu.com") );
    if(data.isEmpty())
    {
        ui->label_neterror->show();
    }
    else
    {
        ui->label_neterror->hide();
    }

    QDateTime timestr = QDateTime::currentDateTime();

    // timestr = timestr.toUTC();

    int timetemp = timestr.toTime_t();
    if(m_time == 0 ||(m_time > 0 && timetemp- m_time <120))return;
    std::string status,hash;
    LOG_WRITE(LOG_INFO,"GetConsensusStatus");
    bool finish= CDpocInfo::Instance().GetConsensusStatus(status,hash);
    if(finish)
        LOG_WRITE(LOG_INFO,"GetConsensusStatus back ",QString::fromStdString(status).toStdString().c_str());
    else
        LOG_WRITE(LOG_INFO,"GetConsensusStatus false ");
    static std::string oldstatus;
    if(status == "1"||status == "4")
    {
        ui->label_punishment->setText(tr("General punishment.Long term abnormal bookkeeping, please standardize bookkeeping behavior."));
        ui->pushButton_outaccount->hide();
        ui->label_shownumtip->hide();
        if(oldstatus != "1" && oldstatus != "4"){
            CMessageBox *msgbox = new CMessageBox();
            msgbox->setMessage(1);
            msgbox->show();
        }

    }
    else if(status == "2"||status == "5")
    {
        ui->label_punishment->setText(tr("Serious punishment You can't continue accounting. Quitting, please be patient."));
        ui->pushButton_outaccount->hide();
        ui->label_shownumtip->hide();
    }
    else if(status == "0")
    {
        // ui->pushButton_outaccount->show();
    }

    oldstatus  = status;

}
void TallyAccount::updateDisplayList(int num)
{

}
void TallyAccount::updateIpcList()
{

    QDateTime time = QDateTime::currentDateTime();
    int timeT = time.toTime_t();
    static int timeOld = 0;
    if(timeOld == timeT)
    {
        timeOld = timeT;
        //LOG_WRITE(LOG_INFO,"TallyAccountList",\
                  QString::number(timeOld).toStdString().c_str(),\
                  QString::number(timeT).toStdString().c_str());
        return;
    }
    LOG_WRITE(LOG_INFO,"TallyAccountList");
    timeOld = timeT;
    ui->tableWidget->clearContents();
    m_bookkeeplist.clear();
    if(walletmodel)
    {
        walletmodel->getbookkeepList(m_bookkeeplist);
        //QWidget * pnewall = new QWidget();
        //pvboxlayoutall = new QVBoxLayout();
        //pnewall->setLayout(pvboxlayoutall);
        //ui->scrollArea->setWidget(pnewall);
        ui->tableWidget->setRowCount(m_bookkeeplist.size());
        for(int i=0;i<m_bookkeeplist.size();i++)
        {
            QString time = QString::fromStdString(m_bookkeeplist.at(i).time);
            QString award = QString::fromStdString(m_bookkeeplist.at(i).award);
            award=award.insert(award.size()-8,".");
            award = award + " IPC";
            QString strBuffer;
            QDateTime datetime = QDateTime::fromTime_t(time.toInt());
            strBuffer = datetime.toString("yyyy-MM-dd hh:mm:ss");
            addItemContent(i,0,strBuffer);
            addItemContent(i,1,award);
            //addnewoneipcmessage(QString::fromStdString(m_bookkeeplist.at(i).time),QString::fromStdString(m_bookkeeplist.at(i).award));//,m_ipclist.at(i).at(2),1,i);
        }
        GetTotalAmount();

    }
    LOG_WRITE(LOG_INFO,"TallyAccountList finish");
}
void TallyAccount::addItemContent(int row, int column, QString content)
{
    QTableWidgetItem *item = new QTableWidgetItem (content);
    item->setTextAlignment(Qt::AlignCenter);
    ui->tableWidget->setItem(row, column, item);
}

void  TallyAccount::setfinishinfo()
{
    ui->pushButton_outaccount->hide();
    ui->label_punishment->setText(tr("Quitting, please be patient."));
    ui->label_shownumtip->hide();

}
void  TallyAccount::GetTotalAmount()
{
    int64_t u32Amount = CDpocInfo::Instance().GetTotalAmount();
    QString strtotal = QString::number(u32Amount);
    if(u32Amount>100000000){
        strtotal.insert(strtotal.size()-8,'.');
    }
    ui->label_totalincome->setText(strtotal);

}

void TallyAccount::getPasswordIfTallyAccountIng()
{
    LOG_WRITE(LOG_INFO,"IsHasLocalAccount");
    if(!CDpocInfo::Instance().IsHasLocalAccount())
    {
        LOG_WRITE(LOG_INFO,"IsHasLocalAccount false");
        return;
    }
    LOG_WRITE(LOG_INFO,"IsHasLocalAccount true");
    QSettings settings;
    int iPasswordErrorNum = settings.value("PasswordErrorNum").toInt();
    if(iPasswordErrorNum >= 5){
        showPwdErrAndStop();
        return;
    }
    QLocale locale;
    while(iPasswordErrorNum++<=5){
        if(!walletmodel->CheckPassword())
        {
            LOG_WRITE(LOG_INFO,"tallyapply","psd error");
            if(iPasswordErrorNum<5){
                if( locale.language() == QLocale::Chinese )
                {
                    QMessageBox::information(this, tr("IPC"),tr("密码错误，请重新输入。"));
                }else{
                    QMessageBox::information(this, tr("IPC"),tr("Password error, please re-enter."));
                }
            }
        }
        else{
            iPasswordErrorNum = 0;
            break;
        }
    }
    if(iPasswordErrorNum >= 5){
        showPwdErrAndStop();
    }


    WalletModel::UnlockContext ctx(walletmodel, true, true);

}
void  TallyAccount::showPwdErrAndStop()
{
    QLocale locale;
    // if(!walletmodel->CheckPassword())
    // {
    bool was_locked = (walletmodel->getEncryptionStatus() == WalletModel::Locked)?1:0;
    if(!was_locked)
        return;

    if( locale.language() == QLocale::Chinese )
    {
        QMessageBox::information(this, tr("IPC"),tr("密码错误，您将终止记账"));
        return;
    }else{
        QMessageBox::information(this, tr("IPC"),tr("Password Error.You will terminate the account book."));
    }
    // }
}
