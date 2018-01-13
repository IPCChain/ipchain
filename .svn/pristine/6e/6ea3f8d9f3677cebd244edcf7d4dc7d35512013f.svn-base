#include "logon.h"
#include "ui_logon.h"
#include <QDesktopWidget>
#include <QPixmap>
#include <QFileDialog>
#include <QPixmapCache>
#include "log/log.h"
int g_loadingtime=0;
logon::logon(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::logon)
{
    ui->setupUi(this);
    ui->label_wait->hide();
    timerEvent(NULL);
    m_nTimerId = startTimer(150);
}

logon::~logon()
{
    killTimer(m_nTimerId);
    delete ui;
}

void logon::on_pushButton_createwallet_pressed()
{
    Q_EMIT createwallet();
}
void logon::showwaitpic()
{
    ui->pushButton_createwallet->hide();
    ui->pushButton_restorefrombackup->hide();
    ui->label_wait->show();
    ui->label_datapath->hide();
    ui->pushButton_changedata->hide();
    ui->label_setdatapath->hide();
}


void logon::on_pushButton_restorefrombackup_pressed()
{
    Q_EMIT gotoRestorePage();
}
void logon::timerEvent( QTimerEvent *event )
{
    QString res;
    g_loadingtime ++;
    int i = g_loadingtime%25;
    res =":/res/png/loading/loading"+QString::number(i)+".png";
    QPixmapCache::clear();
    QPixmapCache::setCacheLimit(1);
    QPixmap pixmap;
    pixmap.load(res);
    ui->label_wait->setPixmap(pixmap);
    ui->label_wait->setScaledContents(true);
}
void logon::setdatapath(QString path)
{
    ui->label_setdatapath->setText(path);
}

void logon::on_pushButton_changedata_pressed()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"), "/", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(!dir.isEmpty()){
        ui->label_setdatapath->setText(dir);
    }
}
QString logon::getdatapath()
{
    return  ui->label_setdatapath->text();
}
void logon::stopLoadingTimer()
{
    killTimer(m_nTimerId);
}
