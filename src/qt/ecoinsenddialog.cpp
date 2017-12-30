#include "ecoinsenddialog.h"
#include "forms/ui_ecoinsenddialog.h"
#include <QMessageBox>
#include "guiutil.h"
#include <qrencode.h>
eCoinSenddialog::eCoinSenddialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::eCoinSenddialog)
{
    ui->setupUi(this);
    ui->label_3->hide();
    ui->qrlabel->hide();
    ui->addlabel->hide();
}

eCoinSenddialog::~eCoinSenddialog()
{
    delete ui;
}
void eCoinSenddialog::setMsg(QString name,QString num,QString add)
{
    if(num.size()>0 && num.indexOf(".") == 0)
    {
        num.insert(0,"0");
    }
    ui->nameEdit->setText(name);
    ui->numEdit->setText(num);
    ui->addlabel->setText(add);
    m_name = name;
    m_num = num;
    m_address = add;
    return;
    std::string str = add.toStdString();
    const char* ch = str.c_str();
    QRcode *code = QRcode_encodeString(ch, 0, QR_ECLEVEL_L, QR_MODE_8, 1);
    if (!code)
    {
        return;
    }
    else
    {
        QImage qrImage = QImage(code->width + 8, code->width + 8, QImage::Format_RGB32);
        qrImage.fill(0xffffff);
        unsigned char *p = code->data;
        for (int y = 0; y < code->width; y++)
        {
            for (int x = 0; x < code->width; x++)
            {
                qrImage.setPixel(x + 4, y + 4, ((*p & 1) ? 0x0 : 0xffffff));
                p++;
            }
        }
        QRcode_free(code);

        QImage qrAddrImage = QImage(200, 200+20, QImage::Format_RGB32);
        qrAddrImage.fill(0xffffff);

        QPainter painter(&qrAddrImage);
        painter.drawImage(0, 0, qrImage.scaled(200, 200));
        QFont font = GUIUtil::fixedPitchFont();
        font.setPixelSize(12);
        painter.setFont(font);
        QRect paddedRect = qrAddrImage.rect();
        paddedRect.setHeight(200+12);
        // painter.drawText(paddedRect, Qt::AlignBottom|Qt::AlignCenter, info.address);
        painter.end();

        ui->qrlabel->setPixmap(QPixmap::fromImage(qrAddrImage));
    }

}
void eCoinSenddialog::on_GoBtn_pressed()
{
    Q_EMIT gotoSendAffrimDialog(m_name,m_num);
}

