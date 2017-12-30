#include "adddetail.h"
#include "ui_adddetail.h"
#include "addresstablemodel.h"
#include <qrencode.h>
#include "guiutil.h"

#include <qpainter.h>>

#include <QAbstractItemDelegate>
#include <QPainter>
AddDetail::AddDetail(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AddDetail)
{
    ui->setupUi(this);
}

AddDetail::~AddDetail()
{
    delete ui;
}
void AddDetail::showEvent(QShowEvent *event)
{
    QString newadd = model->getAddressTableModel()->addRow( AddressTableModel::Receive,//wangdandan20170821
                                                           "111111111111111",
                                                            "111111111111111111111111");
    std::string str = newadd.toStdString();

    const char* ch = str.c_str();

    ui->txlabel->setText(QString::fromStdString(str));

    QRcode *code = QRcode_encodeString(ch, 0, QR_ECLEVEL_L, QR_MODE_8, 1);
    if (!code)
    {
        return;
    }
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

    QImage qrAddrImage = QImage(300, 300+20, QImage::Format_RGB32);
    qrAddrImage.fill(0xffffff);

    QPainter painter(&qrAddrImage);
    painter.drawImage(0, 0, qrImage.scaled(300, 300));
    QFont font = GUIUtil::fixedPitchFont();
    font.setPixelSize(12);
    painter.setFont(font);
    QRect paddedRect = qrAddrImage.rect();
    paddedRect.setHeight(300+12);
    // painter.drawText(paddedRect, Qt::AlignBottom|Qt::AlignCenter, info.address);
    painter.end();

    ui->qrlabel->setPixmap(QPixmap::fromImage(qrAddrImage));
}
void AddDetail::setModel(WalletModel *_model)
{
    this->model = _model;
}

void AddDetail::on_backButton_pressed()
{
    Q_EMIT back();
}
