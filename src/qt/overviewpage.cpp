// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "bitcoinunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "transactionfilterproxy.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"
#include "transactionview.h"//20170807
#include "transactionrecord.h"
#include "infowidget.h"
#include "sendhistory.h"
#include "recvhistory.h"
#include "sendipchistory.h"
#include "recvipchistory.h"
#include "sendtokenhistory.h"
#include "recvtokenhistory.h"
#include <QAbstractItemDelegate>
#include <QPainter>
#include <QMenu>
#include <QMessageBox>
#include <sstream>
#include "log/log.h"


//#include <qt4/QtGui/qabstractitemview.h>
#include "transactiondescdialog.h"
#define DECORATION_SIZE 54
#define NUM_ITEMS 4//5

QModelIndex selectindex_ ;

CAmount _balance =0 ;


class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(const PlatformStyle *_platformStyle, QObject *parent=nullptr):
        QAbstractItemDelegate(parent), unit(BitcoinUnits::IPC),
        platformStyle(_platformStyle)
    {
    }
    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {

        painter->save();
        QIcon icon = qvariant_cast<QIcon>(index.data(TransactionTableModel::RawDecorationRole));
        //guolvguolvguolv
        int type = index.data(TransactionTableModel::TypeRole).toInt();
        // QDateTime
        QString date = index.data(TransactionTableModel::DateRole).toString();//.toDateTime();
        bool involvesWatchAddress = index.data(TransactionTableModel::WatchonlyRole).toBool();
        QString address = index.data(TransactionTableModel::AddressRole).toString();
        QString labelr = index.data(TransactionTableModel::LabelRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        // QString amountText = BitcoinUnits::formatWithUnit(0, amount, true, BitcoinUnits::separatorAlways);

        QString amountText = BitcoinUnits::formatWithUnit4(0, amount, true, BitcoinUnits::separatorAlways);
        if("0.0000 IPC"==amountText || "+0.0000 IPC"==amountText||"-0.0000 IPC"==amountText)
        {
            amountText = BitcoinUnits::formatWithUnit(0, amount, true, BitcoinUnits::separatorAlways);

        }

        bool confirmed = index.data(TransactionTableModel::IsConfirmedRole).toBool();

        if(!confirmed)
        {
            // amountText = QString("[") + amountText + QString("]");
        }
        int status = index.data(TransactionTableModel::StatusRole).toInt();


        int  watchOnlyFilter =index.data(TransactionFilterProxy::WatchOnlyFilter_No).toInt();
        int  watchOnlyFilter1 =index.data(TransactionFilterProxy::WatchOnlyFilter_Yes).toInt();

        if(0 == amount )
        {

        }

        QString name=index.data(TransactionTableModel::IPCTitle).toString();
        QString ecoinname=index.data(TransactionTableModel::eCoinType).toString();
        QString ecoinnum = index.data(TransactionTableModel::eCoinNum).toString();


        QString label;


        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        QRect mainRect = option.rect;
        //        QRect decorationRect(mainRect.left()+2,mainRect.top()+15, DECORATION_SIZE, DECORATION_SIZE);

        QRect decorationRect(mainRect.left()+7,mainRect.top()+25, 41, 41);
        int halfheight = (mainRect.height() - 2*ypad)/2;

        QRect amountRect(mainRect.left()+ xspace*0.7, mainRect.top()-ypad+halfheight/2, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace*0.7, mainRect.top()+ypad+halfheight*4/5, mainRect.width() - xspace, halfheight);
        QRect labelRect(mainRect.left() -xspace*1.7, mainRect.top()+ypad+halfheight/3, mainRect.width() - xspace, halfheight);
        //   QRect checkRect(mainRect.left() +xspace*4.39, mainRect.top()+ypad, 9,71);
        QRect checkRect(mainRect.right() -8, mainRect.top()+ypad, 9,71);

        QIcon icon1,icon2;

        if( TransactionRecord::Other == type ||  TransactionRecord::SendToAddress== type || TransactionRecord::SendToOther== type )
        {
            if(!confirmed)
            {
                label =tr("IsSended");
                //label = tr("IsSend");
                icon1 = QIcon(":/res/png/coinsend.png");
            }
            else
            {
                label =tr("IsSended");
                //label =tr("IsSending");
                icon1 = QIcon(":/res/png/coinsend.png");
            }

        }
        else if( TransactionRecord::SendToSelf == type || TransactionRecord::Senddeposit == type)
        {
            label =tr("IsSended");
            //label =tr("sendtoself");
            icon1 = QIcon(":/res/png/coinsend.png");
        }
        else if (TransactionRecord::SendeCoin == type || TransactionRecord::SendeCointoself == type)
        {
            label =tr("IsSended");
            // label =tr("sendToken");
            icon1 = QIcon(":/res/png/ecoinsend.png");
        }
        else if (TransactionRecord::SendCoin == type )
        {
            label =tr("IsSended");
            // label =tr("sendcoin");
            icon1 = QIcon(":/res/png/coinsend.png");
        }
        else if (TransactionRecord::RecveCoin == type)
        {
            label = tr("IsRecved");
            // label =tr("recvecoin");
            icon1 = QIcon(":/res/png/ecoinrecv.png");
        }
        else if(TransactionRecord::SendIPC == type)
        {
            if(!confirmed)
            {
                label =tr("IsSended");
                //label =tr("IsSendIPC");
                icon1 = QIcon(":/res/png/ipcsend.png");
            }
            else
            {
                label = tr("IsSended");
                // label=tr("IsSendingIPC");
                icon1 = QIcon(":/res/png/ipcsend.png");
            }
        }
        else if(TransactionRecord::RecvIPC == type)
        {
            if(!confirmed)
            {
                label = tr("IsRecved");
                // label = tr("IsRecvIPC");
                icon1 = QIcon(":/res/png/ipcrecv.png");
            }
            else
            {
                label = tr("IsRecved");
                // label =tr("IsRecvingIPC");
                icon1 = QIcon(":/res/png/ipcrecv.png");
            }
        }


        else if (TransactionRecord::Sendbookkeep == type)
        {
            label = tr("IsSended");
            // label =tr("sendbookkeep");
            icon1 = QIcon(":/res/png/coinsend.png");
        }
        else if (TransactionRecord::Recvbookkeep == type)
        {
            label = tr("IsRecved");
            //label =tr("recvbookkeep");
            icon1 = QIcon(":/res/png/coinsend.png");
        }

        //
        else if(TransactionRecord::RecveCoin == type  )
        {
            if(!confirmed)
            {
                label = tr("IsRecved");
                // label = tr("IsRecvecoin");
                icon1 = QIcon(":/res/png/coinrecv.png");
            }
            else
            {

                label = tr("IsRecved");
                //                label = tr("IsRecvingecoin");
                icon1 = QIcon(":/res/png/coinrecv.png");
            }

        }
        else if(  TransactionRecord::Generated||TransactionRecord::Recvbookkeep == type ||TransactionRecord::RecvCoin == type ||TransactionRecord::RecvFromOther == type || TransactionRecord::RecvWithAddress ==type || TransactionRecord::Recvdeposit == type)
        {
            if(!confirmed)
            {
                label = tr("IsRecved");
                //label = tr("IsRecv");
                icon1 = QIcon(":/res/png/coinrecv.png");
            }
            else
            {

                label = tr("IsRecved");
                //label = tr("IsRecving");
                icon1 = QIcon(":/res/png/coinrecv.png");
            }

        }
        icon1.paint(painter, mainRect);


        //        icon = platformStyle->SingleColorIcon(icon);//red
        icon.paint(painter, decorationRect);

        QSpacerItem* horizontalSpacer = new QSpacerItem(40,20,QSizePolicy::Expanding);



        QStyleOptionViewItem itemOption(option);
        // remove the focus state
        //
        if (itemOption.state & QStyle::State_HasFocus &&  selectindex_ == index)
        {

            icon2 =QIcon(":/res/png/triangle.png");
            icon2.paint(painter, checkRect);
           // itemOption.state ^= QStyle::State_HasFocus;
          //   itemOption.state = itemOption.state ^ QStyle::State_HasFocus;
        }


        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(value.canConvert<QBrush>())
        {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }
        painter->setPen(QColor(255, 255,255));
        //painter->setPen(foreground);
        QRect boundingRect;

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
        }
        // painter->setPen(foreground);
        painter->setPen(QColor(255, 255,255));
        if(TransactionRecord::RecvIPC == type || TransactionRecord::SendIPC == type)
        {

            if(!confirmed)
            {
                painter->setPen(QColor(255, 0,0));
            }
            else
            {
                painter->setPen(QColor(255, 255,255));
            }
            painter->drawText(amountRect, Qt::AlignRight|Qt::AlignTop, name);
        }
        else if(TransactionRecord::RecveCoin == type )
        {
            if(!confirmed)
            {
                painter->setPen(QColor(255,0,0));
            }
            else
            {
                painter->setPen(QColor(255, 255,255));
            }
            painter->drawText(amountRect, Qt::AlignRight|Qt::AlignTop, "+"+ecoinnum+" "+ecoinname);
        }
        else if(TransactionRecord::SendeCoin == type)
        {
            if(!confirmed)
            {
                painter->setPen(QColor(255,0,0));
            }
            else
            {
                painter->setPen(QColor(255, 255,255));
            }
            painter->drawText(amountRect, Qt::AlignRight|Qt::AlignTop, "-"+ecoinnum+" "+ecoinname);
        }
        else{
            if(!confirmed)
            {
                painter->setPen(QColor(255,0,0));
            }
            else
            {
                painter->setPen(QColor(255, 255,255));
            }
            painter->drawText(amountRect, Qt::AlignRight|Qt::AlignTop, amountText);
        }
        painter->setPen(option.palette.color(QPalette::Text));

        painter->setPen(QColor(255, 255,255));
        // painter->drawText(addressRect, Qt::AlignRight|Qt::AlignBottom, GUIUtil::dateTimeStr(date));
        painter->drawText(addressRect, Qt::AlignRight|Qt::AlignBottom, date);

        QFont font;
        font.setPixelSize(16);
        painter->setFont(font);
        painter->setPen(QColor(255, 255,255));

        painter->drawText(labelRect, Qt::AlignRight|Qt::AlignBottom, label);
        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {

        return QSize(90, 90);
    }


    int unit;
    bool isselected;
    const PlatformStyle *platformStyle;



};

#include "overviewpage.moc"

OverviewPage::OverviewPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::overviewpage),
    clientModel(0),
    walletModel(0),
    currentBalance(-1),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    currentWatchOnlyBalance(-1),
    currentWatchUnconfBalance(-1),
    currentWatchImmatureBalance(-1),
    wWidget(NULL),
    txdelegate(new TxViewDelegate(platformStyle, this))
{
    ui->setupUi(this);
    // use a SingleColorIcon for the "out of sync warning" icon
    QIcon icon = platformStyle->SingleColorIcon(":/icons/warning");
    icon.addPixmap(icon.pixmap(QSize(64,64), QIcon::Normal), QIcon::Disabled); // also set the disabled icon because we are using a disabled QPushButton to work around missing HiDPI support of QLabel (https://bugreports.qt.io/browse/QTBUG-42503)

    ui->listTransactions->setEditTriggers(QAbstractItemView::AnyKeyPressed
                                          | QAbstractItemView::DoubleClicked);
    ui->listTransactions->setAutoScroll(true);
    ui->listTransactions->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->listTransactions->setSpacing(3);
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    //  ui->listTransactions->setMinimumHeight(NUM_ITEMS * (30 + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    // showOutOfSyncWarning(true);
    singleton1 = InfoWidget::GetInstance();
    ui->verticalLayout_2->addWidget(singleton1);
    wWidget = singleton1;

    singleton1->setuserRequest(true);
    singleton1->showHide(false);
    ui->showdetailButton->hide();


}



void OverviewPage::chooseDate(int idx)
{
    if(!this->walletModel->getTransactionTableModel())
        return;
    QDate current = QDate::currentDate();
}
void OverviewPage::showDetails(QModelIndexList index)
{
    QModelIndexList selection =index; //ui->tableView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        TransactionDescDialog *dlg = new TransactionDescDialog(selection.at(0));
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    }
}
void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        Q_EMIT transactionClicked(filter->mapToSource(index));

}

void OverviewPage::handleOutOfSyncWarningClicks()
{
    Q_EMIT outOfSyncWarningClicked();
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance, const CAmount& depositBalance)
{
/*
     updatetime++;
    QModelIndex index1 = walletModel->getTransactionTableModel()->index((selectindex_.row()+updatetime),0);

           selectindex_ = index1;
        //    ui->listTransactions->setCurrentIndex(selectindex_);


            LOG_WRITE(LOG_INFO,"whatwhat",QString::number(selectindex_.row()).toStdString().c_str());
            if(selectindex_.row()!=-1)
            {
             //    on_listTransactions_clicked(selectindex_);
            //    updatetime =0;
            }

 ui->listTransactions->update();
*/


       // LOG_WRITE(LOG_INFO,"3333333333333333333333333333333");
    walletModel->getCurrentTime("begin setBalance" );
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    unit = 0;


    _balance = balance;


    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;
    currentdepositBalance  = depositBalance;



    QString a = BitcoinUnits::formatWithUnit2(unit, depositBalance, false, BitcoinUnits::separatorNever);
    QString b;//= BitcoinUnits::formatWithUnit2(unit, balance-depositBalance, false, BitcoinUnits::separatorNever);
    QString c ;//=BitcoinUnits::formatWithUnit2(unit, immatureBalance+unconfirmedBalance, false, BitcoinUnits::separatorNever);


    if(depositBalance > 0)
    {
        ui->labeldetain->setVisible(true);
        ui->labeldetainvalue->setVisible(true);
        ui->labeldetainvalue->setText(BitcoinUnits::formatWithUnit2(unit, depositBalance, false, BitcoinUnits::separatorAlways));


        if(walletModel->GetDepth() < 8)
        {
            ui->labelBalance->setText(BitcoinUnits::formatWithUnit2(unit, balance, false, BitcoinUnits::separatorAlways));
            ui->labelImmature->setText(BitcoinUnits::formatWithUnit2(unit, immatureBalance+unconfirmedBalance-depositBalance, false, BitcoinUnits::separatorAlways));
            b= BitcoinUnits::formatWithUnit2(unit, balance, false, BitcoinUnits::separatorNever);
            c=BitcoinUnits::formatWithUnit2(unit, immatureBalance+unconfirmedBalance-depositBalance, false, BitcoinUnits::separatorNever);
        }
        else
        {
            ui->labelBalance->setText(BitcoinUnits::formatWithUnit2(unit, balance-depositBalance, false, BitcoinUnits::separatorAlways));
            ui->labelImmature->setText(BitcoinUnits::formatWithUnit2(unit, immatureBalance+unconfirmedBalance, false, BitcoinUnits::separatorAlways));
            b= BitcoinUnits::formatWithUnit2(unit, balance-depositBalance, false, BitcoinUnits::separatorNever);
            c=BitcoinUnits::formatWithUnit2(unit, immatureBalance+unconfirmedBalance, false, BitcoinUnits::separatorNever);

        }
    }
    else
    {
        ui->labeldetain->setVisible(false);
        ui->labeldetainvalue->setVisible(false);
        ui->labelBalance->setText(BitcoinUnits::formatWithUnit2(unit, balance, false, BitcoinUnits::separatorAlways));
        ui->labelImmature->setText(BitcoinUnits::formatWithUnit2(unit, immatureBalance+unconfirmedBalance, false, BitcoinUnits::separatorAlways));
        b= BitcoinUnits::formatWithUnit2(unit, balance, false, BitcoinUnits::separatorNever);
        c=BitcoinUnits::formatWithUnit2(unit, immatureBalance+unconfirmedBalance, false, BitcoinUnits::separatorNever);


    }
    double deposit = a.mid(0,a.length()-3).toDouble();
    double blance = b.mid(0,b.length()-3).toDouble();
    double immature = c.mid(0,c.length()-3).toDouble();

    double total = deposit+blance+immature;

    char buf[100]={'\0'};
    sprintf(buf,"%.2lf",total);

    std::string str(buf, sizeof(buf));

    //std::ostringstream oss;  //创建一个格式化输出流
    // oss<<total;
    ui->labelTotal->setText(QString::fromStdString(buf)+" IPC");


    //   ui->labelTotal->setText(BitcoinUnits::formatWithUnit2(unit, balance + unconfirmedBalance + immatureBalance, false, BitcoinUnits::separatorNever));
    //   ui->labelTotal->setText(BitcoinUnits::formatWithUnit(unit, balance + unconfirmedBalance + immatureBalance, false, BitcoinUnits::separatorAlways));
    bool showImmature = immatureBalance != 0;
    bool showWatchOnlyImmature = watchImmatureBalance != 0;
    walletModel->getCurrentTime("end setBalance" );
}


void OverviewPage::setClientModel(ClientModel *model)
{

    this->clientModel = model;

}


void OverviewPage::setWalletModel(WalletModel *model)
{
     this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        filter.reset(new TransactionFilterProxy());
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);

        filter->setWatchOnlyFilter(TransactionFilterProxy::WatchOnlyFilter_No);//20171001
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter.get());

        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);
        ui->listTransactions->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(),
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance(),model->GetDetained());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)),Qt::QueuedConnection);

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        singleton1->setWalletModel(this->walletModel);
    }

    updateDisplayUnit();
}


void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if(currentBalance != -1)
        {
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance,
                       currentWatchOnlyBalance, currentWatchUnconfBalance, currentWatchImmatureBalance,currentdepositBalance);
        }

        // Update txdelegate->unit with the current unit
        txdelegate->unit = walletModel->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->setEditTriggers(QAbstractItemView::AnyKeyPressed
                                              | QAbstractItemView::DoubleClicked
                                              |QAbstractItemView::EditKeyPressed );
        ui->listTransactions->edit(selectindex_);

        ui->listTransactions->update();
    }
}


void OverviewPage::on_listTransactions_clicked(const QModelIndex &index)
{
   LOG_WRITE(LOG_INFO,"55555555555555555555555555555");
    selectindex_ = index;
    updatetime =0;
LOG_WRITE(LOG_INFO,"on_listTransactions_clicked",QString::number(selectindex_.row()).toStdString().c_str());
    txdelegate->isselected = true;

    ui->listTransactions->update();
    if(filter)
        typerole= index.data(TransactionTableModel::TypeRole).toInt();

    int tag;
    if(TransactionRecord::Type::SendIPC== typerole)
    {
        tag = 1;
    }
    else if(TransactionRecord::Type::RecvIPC== typerole)
    {
        tag = 2;
    }
    else if(TransactionRecord::Sendbookkeep == typerole )
    {
        tag = 3;
    }
    else if(TransactionRecord::Recvbookkeep == typerole)
    {
        tag =4;
    }
    else if(TransactionRecord::SendeCointoself==typerole || TransactionRecord::SendeCoin==typerole)
    {
        tag = 5;
    }
    else if( TransactionRecord::RecveCoin== typerole)
    {
        tag = 6;
    }
    else if(TransactionRecord::SendCoin == typerole ||TransactionRecord::SendToAddress== typerole
            || TransactionRecord::SendToOther== typerole || TransactionRecord::SendToSelf == typerole || TransactionRecord::Senddeposit == typerole)
    {
        tag = 7;
    }
    else if( TransactionRecord::Recvdeposit == typerole || TransactionRecord::RecvCoin == typerole || TransactionRecord::Generated == typerole|| TransactionRecord::Other == typerole)
    {
        tag = 8;
    }
    Q_EMIT transactionnnClicked(filter->mapToSource(index),tag);

}
void OverviewPage::addinfo(InfoWidget *infopage){
    if(wWidget !=NULL)
    {
        wWidget->hide();
    }

    wWidget = infopage;

    ui->verticalLayout_2->addWidget(infopage);

    infopage->showHide(false);
}
void OverviewPage::addsendpage(sendhistory *sendpage)
{
    if(wWidget !=NULL)
    {
        wWidget->hide();
    }

    ui->verticalLayout_2->addWidget(sendpage);
    wWidget = sendpage;
    sendpage->showVisual(true);

    ui->showdetailButton->show();
}

void OverviewPage::addrecv(RecvHistory *recvpage)
{
    if(wWidget !=NULL)
    {
        wWidget->hide();
    }

    ui->verticalLayout_2->addWidget(recvpage);
    wWidget = recvpage;
    recvpage->showVisual(true);
    ui->showdetailButton->show();
}
void OverviewPage::addsendipcpage(sendipchistory *sendipcpage)
{

    if(wWidget !=NULL)
    {
        wWidget->hide();
    }

    ui->verticalLayout_2->addWidget(sendipcpage);
    wWidget = sendipcpage;
    sendipcpage->showVisual(true);
    ui->showdetailButton->show();
}

void OverviewPage::addsendtokenpage(SendTokenHistory *sendTokenPage)
{
    if(wWidget !=NULL)
    {
        wWidget->hide();
    }

    ui->verticalLayout_2->addWidget(sendTokenPage);
    wWidget = sendTokenPage;
    sendTokenPage->showVisual(true);
    ui->showdetailButton->show();

}
void OverviewPage::addrecvtokenpage(RecvTokenHistory *recvTokenPage)
{
    if(wWidget !=NULL)
    {
        wWidget->hide();
    }

    ui->verticalLayout_2->addWidget(recvTokenPage);
    wWidget = recvTokenPage;
    recvTokenPage->showVisual(true);
    ui->showdetailButton->show();
}

void OverviewPage::addrecvipcpage(recvipchistory *recvipcpage)
{
    if(wWidget !=NULL)
    {
        wWidget->hide();
    }

    ui->verticalLayout_2->addWidget(recvipcpage);
    wWidget = recvipcpage;
    recvipcpage->showVisual(true);
    ui->showdetailButton->show();
}
void OverviewPage::on_showdetailButton_pressed()
{
    QModelIndex index = this->walletModel->getTransactionTableModel()->index(-1,-1);
  // selectindex_ = index_;
    ui->showdetailButton->hide();
    addinfo(singleton1);

}
