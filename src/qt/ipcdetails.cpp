#include "ipcdetails.h"
#include "ui_ipcdetails.h"
#include "titlestyle.h"
#include "walletmodel.h"
#include <QPainter>
#include <QFileInfo>
#include <QFileDialog>
#include <QLocale>
#include <QMessageBox>
QStringList type = QStringList()<<QObject::tr("patent")<<QObject::tr("trademark")\
                               <<QObject::tr("Electronic document")<<QObject::tr("Photo")<<QObject::tr("Journalism")\
                              <<QObject::tr("video")<<QObject::tr("audio frequency")<<QObject::tr("security code");
QStringList type_chinese = QStringList()<<"专利"<<"商标"\
                                       <<"电子文档"<<"照片"<<"新闻"\
                                      <<"视频"<<"音频"<<"防伪码";

IpcDetails::IpcDetails(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IpcDetails),walletModel(NULL)
{
    ui->setupUi(this);
    QPalette patime;
    patime.setColor(QPalette::WindowText,Qt::gray);
    ui->label_t7->setPalette(patime);
    ui->label_t8->setPalette(patime);
}

IpcDetails::~IpcDetails()
{
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

    if(walletModel)
    {
        QStringList back = walletModel->getInfoFromIpcList(index);
        if(back.size()>=12)
        {
            ui->label_t1->setText(back.at(0));
            //ui->label_t2->setText(back.at(1));
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
            if("ownership"==back.at(4)){
                ui->label_t5->setText(tr("ownership"));
            }else{
                ui->label_t5->setText(tr("Use right"));
            }
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
                ui->pushButton_transfer->show();
            }
            else
            {
                ui->pushButton_transfer->hide();

            }
            if(back.at(11)=="1")
            {
                ui->pushButton_authorization->show();
            }
            else
            {
                ui->pushButton_authorization->hide();
            }

            m_strExclusive = back.at(12);


            if(back.at(8) == "")
            {
                ui->label_t9->hide();
                ui->label_18->hide();
                ui->line_9->hide();
            }else{
                ui->label_t9->show();
                ui->label_18->show();
                ui->line_9->show();
            }
        }

    }
}



void IpcDetails::on_pushButton_toimage_pressed()
{
    QString file_full, file_name, file_path;
    QFileInfo fi;
    file_full = QFileDialog::getSaveFileName(this,"","/","image Files(*.png)");
    if(file_full == "")
    {
        return;
    }
    file_full=file_full+".png";
    QLocale locale;
    bool isChineseLanguage = false;
    if( locale.language() == QLocale::Chinese )
    {
        isChineseLanguage = true;
    }
    QImage image ;
    if(isChineseLanguage)
        image = QPixmap(":res/png/ipcinfo.png").toImage();
    else
        image = QPixmap(":res/png/ipcinfoEnglish.png").toImage();
    QPainter painter(&image);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);

    QPen pen = painter.pen();
    pen.setColor(Qt::black);
    QFont font = painter.font();
    font.setFamily("SimplifiedChinese");
    //font.setBold(true);//加粗
    font.setPixelSize(17);

    painter.setPen(pen);
    painter.setFont(font);
    QString label_t1_text = ui->label_t1->text();
    painter.drawText(174,264+17,label_t1_text);
    QString label_t2_text = ui->label_t2->text();
    QStringList type = QStringList()<<tr("patent")<<tr("trademark")\
                                   <<tr("Electronic document")<<tr("Photo")<<tr("Journalism")\
                                  <<tr("video")<<tr("audio frequency")<<tr("security code");
    int index = type.indexOf(label_t2_text);
    if(index==0)
    {
        if(isChineseLanguage)
            painter.drawText(174,298+17,"专利");
        else
            painter.drawText(174,298+17,"patent");
    }else if(index==1)
    {
        if(isChineseLanguage)
            painter.drawText(174,298+17,"商标");
        else
            painter.drawText(174,298+17,"trademark");
    }else if(index==2)
    {
        if(isChineseLanguage)
            painter.drawText(174,298+17,"电子文档");
        else
            painter.drawText(174,298+17,"Electronic document");

    }else if(index==3)
    {
        if(isChineseLanguage)
            painter.drawText(174,298+17,"照片");
        else
            painter.drawText(174,298+17,"Photo");
    }else if(index==4)
    {
        if(isChineseLanguage)
            painter.drawText(174,298+17,"新闻");
        else
            painter.drawText(174,298+17,"Journalism");
    }else if(index==5)
    {
        if(isChineseLanguage)
            painter.drawText(174,298+17,"视频");
        else
            painter.drawText(174,298+17,"video");
    }else if(index==6)
    {
        if(isChineseLanguage)
            painter.drawText(174,298+17,"音频");
        else
            painter.drawText(174,298+17,"audio frequency");
    }else if(index==7)
    {
        if(isChineseLanguage)
            painter.drawText(174,298+17,"防伪码");
        else
            painter.drawText(174,298+17,"security code");
    }

    QString label_t5_text = ui->label_t5->text();
    if(isChineseLanguage){
        if(label_t5_text == tr("ownership")){
            painter.drawText(174,332+17,"所有权");
        }
        else{
            painter.drawText(174,332+17,"使用权");
        }
    }else{
        painter.drawText(174+100,332+17,label_t5_text);
    }


    QString label_t3_text = ui->label_t3->text();
    //if(label_t3_text == "forever")label_t3_text= "永久";
    if(isChineseLanguage){
        painter.drawText(174,366+17,label_t3_text);
    }else{
        painter.drawText(174+50,366+17,label_t3_text);
    }
    QString label_t4_text = ui->label_t4->text();
    if(isChineseLanguage){
        if(label_t4_text == "forever")label_t4_text= "永久";
        painter.drawText(174,400+17,label_t4_text);
    }else{
        painter.drawText(174+50,400+17,label_t4_text);
    }




    QString label_t6_text = ui->label_t6->text();
    if(isChineseLanguage){
        if(label_t6_text == tr("can authorization")){
            painter.drawText(174,434+17,"可再授权");
        }
        else{
            painter.drawText(174,434+17,"不可再授权");
        }
    }else{
        painter.drawText(174+100,434+17,label_t6_text);
    }

    if(label_t5_text == tr("ownership")){
        if(isChineseLanguage){
            painter.drawText(189,468+17,"无");
        }else{
            painter.drawText(189+100,468+17,"nothing");
        }
    }
    else{
        if(m_strExclusive == "0"){
            if(isChineseLanguage)
                painter.drawText(189,468+17,"普通授权");
            else
                painter.drawText(189+100,468+17,"general authorization");
        }
        else{
            if(isChineseLanguage)
                painter.drawText(189,468+17,"独家授权");
            else
                painter.drawText(189+100,468+17,"exclusive license");
        }
    }

    QString label_t7_text = ui->label_t7->text();
    painter.drawText(174,502+17,label_t7_text);


    painter.drawText(236-20,606+17,label_t3_text);


    image.save(file_full,0);

}
QString IpcDetails::getStartTime()
{
    return ui->label_t3->text();
}
