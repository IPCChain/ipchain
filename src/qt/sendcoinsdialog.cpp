// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sendcoinsdialog.h"
#include "ui_sendcoinsdialog.h"
#include "addresstablemodel.h"
#include "ipchainunits.h"
#include "clientmodel.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "walletmodel.h"
#include "base58.h"
#include "chainparams.h"
#include "wallet/coincontrol.h"
#include "validation.h"
#include "ui_interface.h"
#include "txmempool.h"
#include "wallet/wallet.h"
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QTextDocument>
#include <QTimer>
#include <QPainter>
#include <time.h>
#include <iostream>
#include "config/bitcoin-config.h" /* for USE_QRCODE */
#include <qrencode.h>

#define SEND_CONFIRM_DELAY   3

SendCoinsDialog::SendCoinsDialog(const PlatformStyle *_platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendCoinsDialog),
    clientModel(0),
    model(0),
    fNewRecipientAllowed(true),
    fFeeMinimized(true),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);
    eTag =0;
    ui->comboBox->addItem(tr("IPC"), IPC);
    ui->comboBox->addItem(tr("mIPC"),mIPC);
    ui->ReceiverEdit->setPlaceholderText(tr("input receiver"));
    ui->labelEdit->setPlaceholderText(tr("input label"));
    ui->CoinEdit->setPlaceholderText(tr("input num"));
    QRegExp regx("^\\d+(\\.\\d+)?$");
    QValidator *validator = new QRegExpValidator(regx, ui->CoinEdit );
    ui->CoinEdit->setValidator( validator );
    connect(ui->comboBox, SIGNAL(activated(int)), this, SLOT(coinUpdate(int)));

}
void SendCoinsDialog::coinUpdate(int idx)
{
    QDate current = QDate::currentDate();
    switch(ui->comboBox->itemData(idx).toInt())
    {
    case IPC:
    {
        eTag = 0;
    }
    break;
    case mIPC:
    {
        eTag = 1;
    }
    break;
    case uIPC:
    {
        eTag = 2;
    }
    break;
    case zhi:
    {
        eTag = 3;
    }
    break;
    default:
    {
        eTag = 0;
    }
    }
}
void SendCoinsDialog::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;
}

void SendCoinsDialog::setModel(WalletModel *_model)
{
    this->model = _model;
}

SendCoinsDialog::~SendCoinsDialog()
{
    QSettings settings;
    delete ui;
}
void SendCoinsDialog::setAddress(const QString &address,const QString &label)
{
    ui->ReceiverEdit->setText(address);
    ui->labelEdit->setText(label);
}
void SendCoinsDialog::on_scanBtn_pressed()
{
    QString filename = GUIUtil::getOpenFileName(this, tr("Select payment request file to open"), "", "", NULL);
    if(filename.isEmpty())
        return;
    QRcode_encodeString("http://www.ipcchain.com/", 2, QR_ECLEVEL_Q, QR_MODE_8, 0);
    QRcode *code = QRcode_encodeString("uri.toUtf8().constData()", 0, QR_ECLEVEL_L, QR_MODE_8, 1);
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
    painter.end();

}
void SendCoinsDialog::on_AddAdsButton_pressed()
{
    Q_EMIT openAddBookPagewidget(model->getAddressTableModel(),2);
}
void SendCoinsDialog::on_GoSettingBtn_pressed()
{
    QString a = ui->ReceiverEdit->text();
    QString label = ui->labelEdit->text();
    QString b = ui->CoinEdit->text();
    if(a != NULL && b != NULL)
    {
        if(!model->validateAddress(a))
        {
            ui->tiplabel->setText(tr("The recipient address is not valid. Please recheck."));
            return;
        }else{
            ui->tiplabel->setText(tr(""));
        }
    }
    else
    {
        ui->tiplabel->setText(tr("input info"));
        return;
    }
    bool isCrypted = this->model->CheckIsCrypted();
    if(isCrypted)
    {

        Q_EMIT openSendAffrimwidget(a,b,label,eTag);
    }
    else
    {
        Q_EMIT openSettingwidget(a,b,label,eTag);
    }
}
void SendCoinsDialog::clearInfo()
{
    ui->ReceiverEdit->setText("");
    ui->labelEdit->setText("");
    ui->CoinEdit->setText("");
}
