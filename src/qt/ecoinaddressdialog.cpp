#include "ecoinaddressdialog.h"
#include "forms/ui_ecoinaddressdialog.h"
#include <qrencode.h>
#include <QPainter>
#include <QFont>
EcoinAddressDialog::EcoinAddressDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EcoinAddressDialog)
{
    ui->setupUi(this);
    connect(ui->pushButton_back, SIGNAL(pressed()), this, SLOT(pushButtonPressedBack()));
}

EcoinAddressDialog::~EcoinAddressDialog()
{
    delete ui;
}
void EcoinAddressDialog::pushButtonPressedBack()
{
    Q_EMIT(back());
}
void EcoinAddressDialog::setAddress(QString address)
{
    std::string str = address.toStdString();
    const char* ch = str.c_str();
    QRcode *code = QRcode_encodeString(ch, 0, QR_ECLEVEL_L, QR_MODE_8, 1);
    ui->label_address->setText(address);
    if (!code)
    {
        //ui->label->setText(tr("Error encoding URI into QR Code."));
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
        QFont font;
        font.setPixelSize(12);
        painter.setFont(font);
        QRect paddedRect = qrAddrImage.rect();
        paddedRect.setHeight(200+12);
        // painter.drawText(paddedRect, Qt::AlignBottom|Qt::AlignCenter, info.address);
        painter.end();

        ui->label_addresspic->setPixmap(QPixmap::fromImage(qrAddrImage));
    }
}
