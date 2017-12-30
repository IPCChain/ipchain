// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "receivecoinsdialog.h"
#include "ui_receivecoinsdialog.h"

#include "addressbookpage.h"
#include "addresstablemodel.h"
#include "bitcoinunits.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "receiverequestdialog.h"
#include "recentrequeststablemodel.h"
#include "walletmodel.h"
#include "addresstablemodel.h"

#include <QAction>
#include <QCursor>
#include <QItemSelection>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextDocument>
#include <QSortFilterProxyModel>
#include<QTableWidgetItem>
#include <iostream>
#include <QClipboard>
//#include <qrencode.h>
//#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h" /* for USE_QRCODE */
//#endif
//#ifdef USE_QRCODE
#include <qrencode.h>
#include "clickqlabel.h"

ReceiveCoinsDialog::ReceiveCoinsDialog(const PlatformStyle *_platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReceiveCoinsDialog),
    columnResizingFixer(0),
    model(0),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);
    contextMenu = new QMenu(this);
    QAction *copyURIAction = new QAction(tr("Copy"), this);

    contextMenu = new QMenu(this);
    contextMenu->addAction(copyURIAction);

    connect(ui->recentRequestsView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showMenu(QPoint)));
    connect(copyURIAction, SIGNAL(triggered()), this, SLOT(copyURI()));
    ClickQLabel* labelben = (ClickQLabel*)ui->AddLabel;
    connect(labelben, SIGNAL(clicked()), this, SLOT(AddLabelClicked()));

   // labelben->setOpenExternalLinks(true);
    //labelben->setText(tr("<a href=\"http://www.baidu.com/\">"));


}
void ReceiveCoinsDialog::setModel(WalletModel *_model)
{

    this->model = _model;

    if(_model && _model->getOptionsModel())
    {
        _model->getRecentRequestsTableModel()->sort(RecentRequestsTableModel::Date, Qt::DescendingOrder);
        //   connect(_model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        //    updateDisplayUnit();

        QTableView* tableView = ui->recentRequestsView;

        tableView->verticalHeader()->hide();
        tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        proxyModel = new QSortFilterProxyModel(this);
        proxyModel->setSourceModel(_model->getAddressTableModel());
        proxyModel->setDynamicSortFilter(true);
        proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

        /*
          switch(tab)
          {
          case ReceivingTab:
              // Receive filter
              proxyModel->setFilterRole(AddressTableModel::TypeRole);
              proxyModel->setFilterFixedString(AddressTableModel::Receive);
              break;
          case SendingTab:
              // Send filter
              proxyModel->setFilterRole(AddressTableModel::TypeRole);
              proxyModel->setFilterFixedString(AddressTableModel::Send);
              break;
          }
          */
        proxyModel->setFilterRole(AddressTableModel::TypeRole);
        proxyModel->setFilterFixedString(AddressTableModel::Receive);

        //20170824


        tableView->setModel(proxyModel);

        // tableView->setModel(_model->getAddressTableModel());//wangdandan
 //tableView->horizontalHeader()
        tableView->setColumnHidden(0,true);
 tableView->setColumnWidth(1,508);
      //  tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents );
      // tableView->setSectionResizeMode(1,QHeaderView::Stretch);
       // tableView->setColumnWidth(1,h-20);//1212121212121
        //   tableView->setTextAlignment(Qt::AlignCenter);
        //    tableView->setTextAlignment(Qt::AlignCenter);
        tableView->verticalHeader()->setDefaultSectionSize(54);//shezhihanggao
        tableView->setAlternatingRowColors(true);
        tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
       // connect(tableView->selectionModel(),
       //         SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
     //           SLOT(recentRequestsView_selectionChanged(QItemSelection, QItemSelection)));
        // Last 2 columns are set by the columnResizingFixer, when the table geometry is ready.
        columnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(tableView, AMOUNT_MINIMUM_COLUMN_WIDTH, DATE_COLUMN_WIDTH, this);

        //20170830
        //   QModelIndex topindex = proxyModel->index(j,0, QModelIndex());//遍历第一行的所有列
        QModelIndex buttomindex=  proxyModel->index(0,1, QModelIndex());

        //   QVariant topvar= topindex.data();
        QVariant bumvar= buttomindex.data();

        //   addnewoneipcmessage(1,topvar.toString(),bumvar.toString(),1);




        QString str = bumvar.toString();;

        char*  ch;

        QByteArray ba = str.toLatin1();

        ch=ba.data();

        //  QRcode_encodeString("http://www.baidu.com/", 2, QR_ECLEVEL_Q, QR_MODE_8, 0);
        QRcode *code = QRcode_encodeString(ch, 0, QR_ECLEVEL_L, QR_MODE_8, 1);
        if (!code)
        {

            ui->label->setText(tr("Error encoding URI into QR Code."));
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

        QImage qrAddrImage = QImage(200, 200, QImage::Format_RGB32);

//        QImage qrAddrImage = QImage(200, 200+20, QImage::Format_RGB32);
        qrAddrImage.fill(0xffffff);

        QPainter painter(&qrAddrImage);
        painter.drawImage(0, 0, qrImage.scaled(200, 200));
        QFont font = GUIUtil::fixedPitchFont();
        font.setPixelSize(25);
        painter.setFont(font);
        QRect paddedRect = qrAddrImage.rect();
        paddedRect.setHeight(200+25);//12
        // painter.drawText(paddedRect, Qt::AlignBottom|Qt::AlignCenter, info.address);
        painter.end();
        //QString temp = str;
        //temp.insert(20,QString("\r\n"));
        ui->AddLabel->setText(str);
        ui->label->setPixmap(QPixmap::fromImage(qrAddrImage));
        //20170830
    }
}

ReceiveCoinsDialog::~ReceiveCoinsDialog()
{
    delete ui;
}
void ReceiveCoinsDialog::setRandString(QString & randString){
    int max = 8;
    QString tmp = QString("0123456789ABCDEFGHIJKLMNOPQRSTUVWZYZ");
    QString str = QString();
    QTime t;
    t= QTime::currentTime();
    qsrand(t.msec()+t.second()*1000);
    for(int i=0;i<max;i++) {
        int ir = qrand()%tmp.length();
        str[i] = tmp.at(ir);
    }
    randString = str;
}
void ReceiveCoinsDialog::on_AddButton_pressed()
{
    QString a,b;
    setRandString(a);
    setRandString(b);

    QString newadd = model->getAddressTableModel()->addRow( AddressTableModel::Receive,//wangdandan20170821
                                                           // a,
                                                           // b);
                                                            "","");
    std::string str = newadd.toStdString();

    const char* ch = str.c_str();
  /*  proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model->getAddressTableModel());
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterRole(AddressTableModel::TypeRole);
    proxyModel->setFilterFixedString(AddressTableModel::Receive);
    ui->recentRequestsView->setModel(proxyModel);
    */
    //  proxyModel->setFilterFixedString(AddressTableModel:QSortFilterProxyModel:Receive);
    //  QRcode_encodeString("http://www.baidu.com/", 2, QR_ECLEVEL_Q, QR_MODE_8, 0);
    QRcode *code = QRcode_encodeString(ch, 0, QR_ECLEVEL_L, QR_MODE_8, 1);
    if (!code)
    {
        ui->label->setText(tr("Error encoding URI into QR Code."));
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
    QImage qrAddrImage = QImage(200, 200, QImage::Format_RGB32);
 //   QImage qrAddrImage = QImage(200, 200+20, QImage::Format_RGB32);
    qrAddrImage.fill(0xffffff);

    QPainter painter(&qrAddrImage);
    painter.drawImage(0, 0, qrImage.scaled(200, 200));
    QFont font = GUIUtil::fixedPitchFont();
    font.setPixelSize(25);//12);//
    painter.setFont(font);
    QRect paddedRect = qrAddrImage.rect();
    paddedRect.setHeight(200+25);//12);
    // painter.drawText(paddedRect, Qt::AlignBottom|Qt::AlignCenter, info.address);
    painter.end();
   // QString temp = QString::fromStdString(str);
   // temp.insert(20,QString("\r\n"));
    ui->AddLabel->setText(QString::fromStdString(str));
    ui->label->setPixmap(QPixmap::fromImage(qrAddrImage));

}

void ReceiveCoinsDialog::on_recentRequestsView_clicked(const QModelIndex &index)
{

    QString newadd = index.data().toString();
    std::string str = newadd.toStdString();

    const char* ch = str.c_str();

    //ui->recentRequestsView->setModel(model->getAddressTableModel());
    //  QRcode_encodeString("http://www.baidu.com/", 2, QR_ECLEVEL_Q, QR_MODE_8, 0);
    QRcode *code = QRcode_encodeString(ch, 0, QR_ECLEVEL_L, QR_MODE_8, 1);
    if (!code)
    {
        ui->label->setText(tr("Error encoding URI into QR Code."));
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


    QImage qrAddrImage = QImage(200, 200, QImage::Format_RGB32);


 //   QImage qrAddrImage = QImage(200, 200+20, QImage::Format_RGB32);
    qrAddrImage.fill(0xffffff);

    QPainter painter(&qrAddrImage);
    painter.drawImage(0, 0, qrImage.scaled(200, 200));
    QFont font = GUIUtil::fixedPitchFont();
    font.setPixelSize(25);//12);
    painter.setFont(font);
    QRect paddedRect = qrAddrImage.rect();
    paddedRect.setHeight(200+25);//12);
    // painter.drawText(paddedRect, Qt::AlignBottom|Qt::AlignCenter, info.address);
    painter.end();
    //QString temp = QString::fromStdString(str);
    //temp.insert(20,QString("\r\n"));
    ui->AddLabel->setText(QString::fromStdString(str));
    ui->label->setPixmap(QPixmap::fromImage(qrAddrImage));


}

QModelIndex ReceiveCoinsDialog::selectedRow()
{
/*
    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model->getAddressTableModel());
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterRole(AddressTableModel::TypeRole);
    proxyModel->setFilterFixedString(AddressTableModel::Receive);
    ui->recentRequestsView->setModel(proxyModel);
*/
    //|| model->getRecentRequestsTableModel()
    if(!proxyModel || !ui->recentRequestsView->selectionModel())
    {


         return QModelIndex();
    }

    QModelIndexList selection = ui->recentRequestsView->selectionModel()->selectedRows(1);
    if(selection.empty())
        return QModelIndex();

    if(selection.size()>1)
        return QModelIndex();
    // correct for selection mode ContiguousSelection
    QModelIndex firstIndex = selection.at(0);

    return firstIndex;
}


void ReceiveCoinsDialog::showMenu(const QPoint &point)
{
    if (!selectedRow().isValid()) {
        return;
    }
    contextMenu->exec(QCursor::pos());
}



// context menu action: copy URI
void ReceiveCoinsDialog::copyURI()
{
    QModelIndex sel = selectedRow();
    if (!sel.isValid()) {
        return;
    }
    QString address = sel.data().toString();
    //   QString address = sel.data(AddressTableModel::Address).toString();

    GUIUtil::setClipboard(address);
}

void ReceiveCoinsDialog::AddLabelClicked()
{
    QClipboard *clipboard = QApplication::clipboard();   //获取系统剪贴板指针
    clipboard->setText(ui->AddLabel->text());
}



