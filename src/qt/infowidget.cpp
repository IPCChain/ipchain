#include "infowidget.h"
#include "ui_infowidget.h"
#include "clientmodel.h"
#include "chainparams.h"
#include <QPropertyAnimation>
#include "overviewpage.h"
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QNetworkRequest>
#include <QProcess>
#include <QLatin1String>
//#include <shellapi.h>
#include <QPixmapCache>
#include "log/log.h"
#include "qthyperlink.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include "json/cJSON.h"

QString version("V0.8.834");

QString getCurrentTime(QString info)
{
    QDateTime current_date_time = QDateTime::currentDateTime();
    QString current_date = current_date_time.toString("yyyy-MM-dd");
    QString current_time = current_date_time.toString("hh:mm:ss.zzz ");
    LOG_WRITE(LOG_INFO,info.toStdString().c_str(),current_time.toStdString().c_str());
    return current_time;
}

#include "walletmodel.h"

bool isfullloaded =false;
InfoWidget* InfoWidget::mm = NULL;
InfoWidget::InfoWidget( QWidget *parent):
    QDialog(parent),
    m_walletModel(NULL),
    bestHeaderDate(QDateTime()),
    ui(new Ui::infowidget),
    m_userRequested(false)
{
    ui->setupUi(this);

    //  mm = this;
    ui->clickinfo->setText("<a href = http://47.92.129.154:3002/insight/feedback>http://47.92.129.154:3002/insight/feedback</a>");
    connect(ui->clickinfo,SIGNAL(clicked(MyQTHyperLink*)),this,SLOT(openUrll()));
    ui->infolabel->setText("<a href = http://47.92.129.154:3002/insight>http://47.92.129.154:3002/insight</a>");
    //设置颜色
    QPalette pa;
    pa.setColor(QPalette::WindowText,Qt::red);
    ui->label_newversion->setPalette(pa);
    connect(ui->infolabel,SIGNAL(clicked(MyQTHyperLink*)),this,SLOT(openUrl1()));
    connect(ui->label_versionhtml,SIGNAL(clicked(MyQTHyperLink*)),this,SLOT(openUrlVersion()));
    getCurrentTime("getVersion  start");
    //
    LOG_WRITE(LOG_INFO,"这个东西会重复调用吗");
    m_versionhtml =  getVersion();
    QString strversionhtml  = m_versionhtml;
    if(strversionhtml.isEmpty()){
        ui->label_newversion->hide();
        ui->label_versionhtml->hide();
    }
    else{
        QString html("<a href = ");
        html=html+strversionhtml+ ">" + strversionhtml + "</a>";
        ui->label_versionhtml->setText(html);
        LOG_WRITE(LOG_INFO,"getVersion ",strversionhtml.toStdString().c_str());
    }
    getCurrentTime("getVersion  end");
}

InfoWidget::~InfoWidget()
{

    delete ui;
}

void InfoWidget::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {

        setNumBlocks(this->clientModel->getNumBlocks(), this->clientModel->getLastBlockDate(),this->clientModel->getVerificationProgress(NULL), false);
        connect(this->clientModel, SIGNAL(numBlocksChanged(int,QDateTime,double,bool)), this, SLOT(setNumBlocks(int,QDateTime,double,bool)));

    }
}
void InfoWidget::setNumBlocks(int count, const QDateTime& blockDate, double nVerificationProgress, bool header)
{

    if(header)
    {
        if(true == this->getuserRequest())
        {
            this->showHide(false);
            this->setKnownBestHeight(count, blockDate);
        }
        else
        {
            this->showHide(true);//, true);
            this->setKnownBestHeight(count, blockDate);
        }

    }
    else
    {
        bool loadingfinish = false;
        if(true == this->getuserRequest())
        {
            this->showHide(false);

            loadingfinish = this->tipUpdate(count, blockDate, nVerificationProgress);//20170821

        }
        else
        {
            loadingfinish = this->tipUpdate(count, blockDate, nVerificationProgress);//20170821
            this->showHide(true);//, true);
        }
        if(loadingfinish){
            m_walletModel->setUpdataFinished();
        }



    }

    /*
    if (!clientModel)
        return;

    enum BlockSource blockSource = clientModel->getBlockSource();
    switch (blockSource) {
    case BLOCK_SOURCE_NETWORK:
        if (header) {
            return;
        }
        break;
    case BLOCK_SOURCE_DISK:
        if (header) {
            // progressBarLabel->setText(tr("Indexing blocks on disk..."));
        } else {
            // progressBarLabel->setText(tr("Processing blocks on disk..."));
        }
        break;
    case BLOCK_SOURCE_REINDEX:
        //progressBarLabel->setText(tr("Reindexing blocks on disk..."));
        break;
    case BLOCK_SOURCE_NONE:
        if (header) {
            return;
        }
        // progressBarLabel->setText(tr("Connecting to peers..."));
        break;
    }


    QDateTime currentDate = QDateTime::currentDateTime();
    qint64 secs = blockDate.secsTo(currentDate);

    tooltip = tr("Processed %n block(s) of transaction history.", "", count);

    // Set icon state: spinning if catching up, tick otherwise
    if(secs < 90*60)
    {
        tooltip = tr("Up to date") + QString(".<br>") + tooltip;
        // labelBlocksIcon->setPixmap(platformStyle->SingleColorIcon(":/icons/synced").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));

#ifdef ENABLE_WALLET

     //   if(infopage)
        {
            //  walletFrame->showOutOfSyncWarning(false);
            if(true == this->getuserRequest())
            {
                this->showHide(false);
            }
            else
            {
                this->showHide(true);//, true);
            }
        }

#endif // ENABLE_WALLET
    }
    else
    {
        QString timeBehindText = GUIUtil::formatNiceTimeOffset(secs);

        tooltip = tr("Catching up...") + QString("<br>") + tooltip;

#ifdef ENABLE_WALLET

       // if(infopage)
        {
            if(true == this->getuserRequest())
            {
                this->showHide(false);
            }
            else
            {
                this->showHide(true);
            }
        }
#endif // ENABLE_WALLET

        tooltip += QString("<br>");
        tooltip += tr("Last received block was generated %1 ago.").arg(timeBehindText);
        tooltip += QString("<br>");
        tooltip += tr("Transactions after this will not yet be visible.");
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");
*/
}

void InfoWidget::openUrll()
{
    bool op =QDesktopServices::openUrl(QUrl(("http://47.92.129.154:3002/insight/feedback")));
}

void InfoWidget::openUrl1()
{
    bool op =QDesktopServices::openUrl(QUrl(("http://47.92.129.154:3002/insight")));
}
void InfoWidget::setKnownBestHeight(int count, const QDateTime& blockDate)
{
    if (count > bestHeaderHeight) {

        bestHeaderHeight = count;
        bestHeaderDate = blockDate;
    }
}
void InfoWidget::fresh(int count, const QDateTime& blockDate, double nVerificationProgress,bool head)
{
    if (head)//20171011
    {
        setKnownBestHeight(count, blockDate);
    }
    else
    {
        bool loadingfinish = tipUpdate(count, blockDate, nVerificationProgress);
        if(loadingfinish){

            m_walletModel->setUpdataFinished();
        }
    }


}

bool InfoWidget::tipUpdate(int count, const QDateTime& blockDate, double nVerificationProgress)
{

    if(bestHeaderHeight != 0)
    {
        if(1 == count/bestHeaderHeight)
        {   if(m_warnlabel_imagename != ":/res/png/loaded.png"){

                    QPixmapCache::clear();

                QPixmapCache::setCacheLimit(1);
                QPixmap p;

                p.load(":/res/png/loaded.png");
                ui->warnlabel->setPixmap(p);


                // ui->warnlabel->setPixmap(QPixmap(":/res/png/loaded.png"));
                m_warnlabel_imagename = ":/res/png/loaded.png";
            }
        }
        else
        {
            if(m_warnlabel_imagename != ":/res/png/loading.png"){


                QPixmapCache::clear();

                QPixmapCache::setCacheLimit(1);
                QPixmap p;

                p.load(":/res/png/loading.png");
                ui->warnlabel->setPixmap(p);
                //ui->warnlabel->setPixmap(QPixmap(":/res/png/loading.png"));
                m_warnlabel_imagename = ":/res/png/loading.png";
            }
        }
    }
    else
    {
        if(m_warnlabel_imagename != ":/res/png/loading.png"){

            QPixmapCache::clear();

            QPixmapCache::setCacheLimit(1);
            QPixmap p;

            p.load(":/res/png/loading.png");
            ui->warnlabel->setPixmap(p);
           // ui->warnlabel->setPixmap(QPixmap(":/res/png/loading.png"));
            m_warnlabel_imagename = ":/res/png/loading.png";
        }

    }


    /*
    if(bestHeaderHeight != 0)
       {
           if(1 == count/bestHeaderHeight)
           {
               ui->warnlabel->setPixmap(QPixmap(":/res/png/loaded.png"));
           }
           else
           {
               ui->warnlabel->setPixmap(QPixmap(":/res/png/loading.png"));
           }
       }
       else
       {
           ui->warnlabel->setPixmap(QPixmap(":/res/png/loading.png"));

       }


  */





    /*
    if(bestHeaderHeight != 0)
    {
        if(1 == count/bestHeaderHeight)
        {   if(m_warnlabel_imagename != ":/res/png/loaded.png"){
              QPixmap p;

              QPixmapCache::clear();

              QPixmapCache::setCacheLimit(1);

               p.load(":/res/png/loaded.png");

               ui->warnlabe->setPixmap(p);


               // ui->warnlabel->setPixmap(QPixmap(":/res/png/loaded.png"));
                m_warnlabel_imagename = ":/res/png/loaded.png";
            }
        }
        else
        {
            if(m_warnlabel_imagename != ":/res/png/loading.png"){

                QPixmap p;

                QPixmapCache::clear();

                QPixmapCache::setCacheLimit(1);

                 p.load(":/res/png/loading.png");

                 ui->warnlabe->setPixmap(p);
               // ui->warnlabel->setPixmap(QPixmap(":/res/png/loading.png"));
                m_warnlabel_imagename = ":/res/png/loaded.png";
            }
        }
    }
    else
    {
        if(m_warnlabel_imagename != ":/res/png/loading.png"){
            QPixmap p;

            QPixmapCache::clear();

            QPixmapCache::setCacheLimit(1);

             p.load(":/res/png/loaded.png");

             ui->warnlabe->setPixmap(p);
          //  ui->warnlabel->setPixmap(QPixmap(":/res/png/loading.png"));
            m_warnlabel_imagename = ":/res/png/loading.png";
        }

    }
*/


    QDateTime currentDate = QDateTime::currentDateTime();

    // keep a vector of samples of verification progress at height
    blockProcessTime.push_front(qMakePair(currentDate.toMSecsSinceEpoch(), nVerificationProgress));
    if (blockProcessTime.size() >= 2)
    {
        static const int MAX_SAMPLES = 5000;
        if (blockProcessTime.count() > MAX_SAMPLES)
            blockProcessTime.remove(MAX_SAMPLES, blockProcessTime.count()-MAX_SAMPLES);
    }

    // show the last block date
    ui->newestblockdate->setText(blockDate.toString(tr("yyyy-MM-dd hh:mm:ss ")));

    QString strBuffer,strBuffer1;
    strBuffer = blockDate.toString("yyyy-MM-dd hh:mm:ss");
    QString tmp=blockDate.date().toString("yyyy-MM-dd hh:mm:ss");


    strBuffer1 = blockDate.toString("yyyy-MM-dd hh:mm:ss");

    // estimate the number of headers left based on nPowTargetSpacing
    // and check if the gui is not aware of the the best header (happens rarely)
    int estimateNumHeadersLeft = bestHeaderDate.secsTo(currentDate) / Params().GetConsensus().nPowTargetSpacing;
    bool hasBestHeader = bestHeaderHeight >= count;
    // show remaining number of blocks
    //  if (estimateNumHeadersLeft < 24 && hasBestHeader) {
    if (hasBestHeader && bestHeaderHeight != 0){//&& bestHeaderDate.isValid()) {
        ui->BlocksLeft->setText(QString::number(bestHeaderHeight - count));
        QString percentnum = QString::number(((float)count/(float)bestHeaderHeight)*100,'f',2);
        ui->percent->setText(percentnum+"%");

        std::string a ="100.00";
        if("100.00" ==QString::number(((float)count/(float)bestHeaderHeight)*100,'f',2))
        {
            isfullloaded = true;
        }
        else
        {
            isfullloaded = false;
        }
        if(percentnum == "100.00"){
            return 1;
        }


    } else {
        ui->percent->setText(tr("loading"));
        ui->BlocksLeft->setText(tr("loading"));
    }
    return 0;
}


bool InfoWidget::getuserRequest()
{
    return m_userRequested;

}
void InfoWidget::setuserRequest(bool userRequested)
{
    m_userRequested = userRequested;

}
void InfoWidget::showHide(bool hide)//,bool userRequested)
{

    if(!hide)
    {
        setVisible(true);
    }
    else
    {
        setVisible(false);
    }

}

QString InfoWidget:: getVersion()
{

    LOG_WRITE(LOG_INFO,"这个东西会重复调用吗323");
    QString url("http://47.92.129.154:3000/getInfo");
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(url)));
    QByteArray responseData;
    QEventLoop eventLoop;
    connect(manager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));
    eventLoop.exec();
    responseData = reply->readAll();
    delete manager;
    QString jsonstr =  QString(responseData);
    std::string stdstrjson = jsonstr.toStdString();
    LOG_WRITE(LOG_INFO,"getVersion ",stdstrjson.c_str());
    LOG_WRITE(LOG_INFO,"local ",version.toStdString().c_str());
    cJSON*root=cJSON_Parse(stdstrjson.c_str());
    if(root){
        cJSON*itemversion=cJSON_GetObjectItem(root,"version");
        if(itemversion){

            cJSON*itemurl =cJSON_GetObjectItem(root,"url");
            QString dataversion(itemversion->valuestring);
            if(itemurl &&  dataversion!= version){
                return QString::fromStdString(itemurl->valuestring);
            }
        }
    }
    return QString("");
}
void InfoWidget::openUrlVersion()
{
    bool back =QDesktopServices::openUrl(QUrl(m_versionhtml));
    if(back)
    {
        LOG_WRITE(LOG_INFO,"openUrl true",m_versionhtml.toStdString().c_str());
    }else{
        LOG_WRITE(LOG_INFO,"openUrl false ",m_versionhtml.toStdString().c_str());
    }
}
