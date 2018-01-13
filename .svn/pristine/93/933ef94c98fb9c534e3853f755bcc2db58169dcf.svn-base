#include "ipcdetails.h"
#include "ui_ipcdetails.h"
#include "walletmodel.h"
#include "guiutil.h"
#include <QPainter>
#include <QFileInfo>
#include <QFileDialog>
#include <QLocale>
#include <QMessageBox>
#include <QTimer>
#include <qrencode.h>
#include <QAbstractTextDocumentLayout>
#include "log/log.h"
extern bool g_bIpchainShutdown;
bool firststart = 1;
bool g_staticfirststart2 = 1;
QStringList type = QStringList()<<QObject::tr("patent")<<QObject::tr("trademark")\
                               <<QObject::tr("Electronic document")<<QObject::tr("Photo")<<QObject::tr("Journalism")\
                              <<QObject::tr("video")<<QObject::tr("audio frequency")<<QObject::tr("security code");
QStringList type_chinese = QStringList()<<"专利"<<"商标"\
                                       <<"电子文档"<<"照片"<<"新闻"\
                                      <<"视频"<<"音频"<<"防伪码";

IpcDetails::IpcDetails(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IpcDetails),walletModel(NULL),m_index(-1)
{
    ui->setupUi(this);
    QPalette patime;
    patime.setColor(QPalette::WindowText,Qt::gray);
    ui->label_t7->setPalette(patime);
    ui->label_t8->setPalette(patime);
    m_nTimerId = startTimer(10000);
    m_pushButton_transfer_show = false;
    m_pushButton_authorization_show = false;
    m_bCanTransaction = false;
}

IpcDetails::~IpcDetails()
{
    killTimer(m_nTimerId);
    delete ui;
}

void IpcDetails::on_pushButton_back_pressed()
{
    Q_EMIT back();
}

void IpcDetails::on_pushButton_transfer_pressed()
{
    Q_EMIT gotoIpcTransferTransactionPage(ui->label_t1->text());
}

void IpcDetails::on_pushButton_authorization_pressed()
{
    Q_EMIT gotoIpcAuthorizationTransactionPage(ui->label_t1->text());
}
void IpcDetails::resetinfo(int index)
{

    ui->pushButton_transfer->hide();
    ui->pushButton_authorization->hide();
    ui->pushButton_toimage->hide();
    m_bCanTransaction = false;
    if(walletModel)
    {
        m_index = index;
        QStringList back = walletModel->getInfoFromIpcList(index);
        if(back.size()>=13)
        {
            QString name = back.at(0);
            if(name.size()>38)
                name.insert(37," ");
            ui->label_t1->setText(name);
            int typenum = back.at(1).toInt();
            QStringList type = QStringList()<<tr("patent")<<tr("trademark")\
                                           <<tr("Electronic document")<<tr("Photo")<<tr("Journalism")\
                                          <<tr("video")<<tr("audio frequency")<<tr("security code");
            if(typenum>=0&&typenum<8)
            {
                ui->label_t2->setText(type.at(typenum));
            }
            else
            {
                ui->label_t2->setText(tr("patent"));
            }
            ui->label_t3->setText(back.at(2));
            if(back.at(3) == "forever")
            {
                ui->label_t4->setText(tr("forever"));
            }else
                ui->label_t4->setText(back.at(3));
            m_ui_label_t5 = back.at(4);
            if("ownership"==back.at(4)){
                ui->label_t5->setText(tr("ownership"));
            }else{
                ui->label_t5->setText(tr("Use right"));
            }
            m_ui_label_t6 = back.at(5);
            if("can authorization"==back.at(5)){
                ui->label_t6->setText(tr("can authorization"));
            }else{
                ui->label_t6->setText(tr("cannot authorization"));
            }

            ui->label_t7->setText(back.at(6));
            ui->label_t8->setText(back.at(7));
            ui->label_t9->setText(back.at(8));

            ui->label_t10->setText(back.at(9));
            if(back.at(10)=="1")
            {
                m_pushButton_transfer_show = true;
            }
            else
            {
                m_pushButton_transfer_show = false;
            }
            if(back.at(11)=="1")
            {
                m_pushButton_authorization_show = true;
            }
            else
            {
                m_pushButton_authorization_show = false;
            }

            m_strExclusive = back.at(12);
            QString canusetime = back.at(13);
            ui->label_inuretime->setText(canusetime);

            if(back.at(8) == "")
            {
                ui->label_t9->hide();
                ui->label_18->hide();
                ui->line_9->hide();
            }else{
                if(firststart){
                    firststart = 0;
                    ui->label_t9->document()->adjustSize();
                }
                int row = ui->label_t9->document()->size().height();
                ui->label_t9->setMinimumHeight(row+5);
                ui->label_t9->setMaximumHeight(row+5);
                ui->label_t9->show();
                ui->label_18->show();
                ui->line_9->show();
            }

            if(walletModel->GetDepthInMainChain(index)>=8){
                bool bcanshow = canTransaction();
                if(m_pushButton_transfer_show&&bcanshow)
                    ui->pushButton_transfer->show();
                if(m_pushButton_authorization_show&&bcanshow)
                    ui->pushButton_authorization->show();
                ui->pushButton_toimage->show();
            }else{
                ui->pushButton_transfer->hide();
                ui->pushButton_authorization->hide();
                ui->pushButton_toimage->hide();
            }
        }
    }
}
void IpcDetails::on_pushButton_toimage_pressed()
{
    QLocale locale;
    QString file_full, file_name, file_path;
    QFileInfo fi;
    file_full = QFileDialog::getSaveFileName(this,"","/","image Files(*.png)");
    if(file_full == "")
    {
        return;
    }
    if(file_full.right(4)!=".png"){
        file_full=file_full+".png";
        QFile mFile(file_full);
        if(mFile.exists())
        {
            QMessageBox::StandardButton reply;
            if( locale.language() == QLocale::Chinese )
                reply = QMessageBox::question(this, "IPC", "该文件名已存在，是否要替换？", QMessageBox::Yes | QMessageBox::No);
            else
                reply = QMessageBox::question(this, "IPC", "The file name already exists. Do you want to replace it?", QMessageBox::Yes | QMessageBox::No);
            if(reply != QMessageBox::Yes)
            {
                return;
            }
        }
    }

    bool isChineseLanguage = false;

    QImage image ;
    image = QPixmap(":res/png/ipcinfoEnglish.png").toImage();
    QPainter painter(&image);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);

    QPen pen = painter.pen();
    pen.setColor(QColor(198,156,109));
    QFont font = painter.font();
    font.setFamily("SimplifiedChinese");

    int fontsize = 18;
    font.setPixelSize(fontsize);

    painter.setPen(pen);
    painter.setFont(font);
    QString label_t1_text = ui->label_t1->text();
    if(label_t1_text.size()<=20){
        painter.drawText(195,286+fontsize,label_t1_text);

    }else if(label_t1_text.size()<=40){
        painter.drawText(195,286-20+fontsize,label_t1_text.mid(0, 20));
        painter.drawText(195,286+fontsize,label_t1_text.mid(20, 20));
    }
    else if(label_t1_text.size()<=60){
            painter.drawText(195,286-40+fontsize,label_t1_text.mid(0, 20));
            painter.drawText(195,286-20+fontsize,label_t1_text.mid(20, 20));
            painter.drawText(195,286+fontsize,label_t1_text.mid(40, 20));
    }
    else{
        painter.drawText(195,286-60+10+fontsize,label_t1_text.mid(0, 20));
        painter.drawText(195,286-40+10+fontsize,label_t1_text.mid(20, 20));
        painter.drawText(195,286-20+10+fontsize,label_t1_text.mid(40, 20));
        painter.drawText(195,286+10+fontsize,label_t1_text.mid(60, 20));
    }

    QString label_t2_text = ui->label_t2->text();
    QStringList type = QStringList()<<tr("patent")<<tr("trademark")\
                                   <<tr("Electronic document")<<tr("Photo")<<tr("Journalism")\
                                  <<tr("video")<<tr("audio frequency")<<tr("security code");
    int index = type.indexOf(label_t2_text);
    int typex = 188;
    int typey = 319+fontsize;
    if(index==0)
    {
        if(isChineseLanguage)
            painter.drawText(typex,typey,"专利");
        else
            painter.drawText(typex,typey,"patent");
    }else if(index==1)
    {
        if(isChineseLanguage)
            painter.drawText(typex,typey,"商标");
        else
            painter.drawText(typex,typey,"trademark");
    }else if(index==2)
    {
        if(isChineseLanguage)
            painter.drawText(typex,typey,"电子文档");
        else
            painter.drawText(typex,typey,"Electronic document");

    }else if(index==3)
    {
        if(isChineseLanguage)
            painter.drawText(typex,typey,"照片");
        else
            painter.drawText(typex,typey,"Photo");
    }else if(index==4)
    {
        if(isChineseLanguage)
            painter.drawText(typex,typey,"新闻");
        else
            painter.drawText(typex,typey,"Journalism");
    }else if(index==5)
    {
        if(isChineseLanguage)
            painter.drawText(typex,typey,"视频");
        else
            painter.drawText(typex,typey,"video");
    }else if(index==6)
    {
        if(isChineseLanguage)
            painter.drawText(typex,typey,"音频");
        else
            painter.drawText(typex,typey,"audio frequency");
    }else if(index==7)
    {
        if(isChineseLanguage)
            painter.drawText(typex,typey,"防伪码");
        else
            painter.drawText(typex,typey,"security code");
    }

    QString label_t5_text =  m_ui_label_t5;//ui->label_t5->text();
    if(isChineseLanguage){
        if(label_t5_text == tr("ownership")){
            painter.drawText(266,350+fontsize,"所有权");
        }
        else{
            painter.drawText(266,350+fontsize,"使用权");
        }
    }else{
        painter.drawText(266,350+fontsize,label_t5_text);
    }

    QString label_t3_text = ui->label_t3->text();
    label_t3_text = QChineseTimeToEnglish(label_t3_text);
    painter.drawText(240,385+fontsize,label_t3_text);

    QString label_t4_text = ui->label_t4->text();
    if(isChineseLanguage){
        if(label_t4_text == "forever")label_t4_text= "永久";
        painter.drawText(202,416+fontsize,label_t4_text);
    }else{
        label_t4_text = QChineseTimeToEnglish(label_t4_text);
        painter.drawText(202,416+fontsize,label_t4_text);
    }

    QString inuretime = ui->label_inuretime->text();
    inuretime = QChineseTimeToEnglish(inuretime);
    painter.drawText(246,451+fontsize,inuretime);

    QString label_t6_text = m_ui_label_t6;//ui->label_t6->text();
    if(isChineseLanguage){
        if(label_t6_text == tr("can authorization")){
            painter.drawText(300,484+fontsize,"可再授权");
        }
        else{
            painter.drawText(300,484+fontsize,"不可再授权");
        }
    }else{
        painter.drawText(300,484+fontsize,label_t6_text);
    }
    QString hashstr = ui->label_t7->text();


    QString str = hashstr;;

    char*  ch;

    QByteArray ba = str.toLatin1();

    ch=ba.data();

    QRcode *code = QRcode_encodeString(ch, 0, QR_ECLEVEL_L, QR_MODE_8, 1);
    if (!code)
    {
    }
    else
    {
        QImage qrImage = QImage(code->width , code->width , QImage::Format_RGB32);
        unsigned char *p = code->data;
        for (int y = 0; y < code->width; y++)
        {
            for (int x = 0; x < code->width; x++)
            {
                qrImage.setPixel(x , y , ((*p & 1) ? 0x363535 : 0xC69C6D));
                p++;
            }
        }
        QRcode_free(code);
        QImage qrAddrImage = QImage(140, 140, QImage::Format_RGB32);
        QPainter  painter1(&qrAddrImage);
        painter1.drawImage(0, 0, qrImage.scaled(140, 140));
        font = GUIUtil::fixedPitchFont();
        font.setPixelSize(0);//25
        painter1.setFont(font);
        QRect paddedRect = qrAddrImage.rect();
        paddedRect.setHeight(140);
        painter1.end();
        painter.drawPixmap(277,569, QPixmap::fromImage(qrAddrImage));
    }
    font.setPixelSize(fontsize);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(183,517+fontsize,hashstr);
    image.save(file_full,0);
}

QString IpcDetails::QChineseTimeToEnglish(QString chtime)
{
    chtime.replace("年","/");
    chtime.replace("日","");
    chtime.replace("月","/");
    chtime.replace("永久","forever");
    return chtime;
}

QString IpcDetails::getStartTime()
{
    return ui->label_t3->text();
}

void IpcDetails::timerEvent( QTimerEvent *event )
{
    if(m_index<0)
        return;
    if(g_bIpchainShutdown)return;
    try{
        int num =     walletModel->GetDepthInMainChain(m_index);
        if(num >=  0)
        {
            QString queren = QString::number(num);
            queren.append("+");
            ui->label_t10->setText(queren);
            if(num >=8)
            {
                if(m_pushButton_transfer_show&&canTransaction())
                    ui->pushButton_transfer->show();
                if(m_pushButton_authorization_show&&canTransaction())
                    ui->pushButton_authorization->show();
                ui->pushButton_toimage->show();

            }
            else if(num < 3){
                ui->label_inuretime->setText(walletModel->GetTimeOfTokenInChain(m_index));
            }
        }
    }
    catch(...){
        LOG_WRITE(LOG_INFO,"IpcDetails::timerEvent Error");
    }
}
bool IpcDetails::canTransaction()
{
    if(!m_bCanTransaction)
        m_bCanTransaction =  walletModel->CanIPCTransaction(m_index);
    return m_bCanTransaction;
}
