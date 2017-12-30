#include "ecoinselectaddress.h"
#include "ui_ecoinselectaddress.h"

#include "walletmodel.h"
#include <QTableView>
#include "guiutil.h"
#include "recentrequeststablemodel.h"
#include "addresstablemodel.h"
#include <iostream>
#include <QSortFilterProxyModel>

eCoinSelectAddress::eCoinSelectAddress(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::eCoinSelectAddress)
{
    ui->setupUi(this);
    m_clientType = 0;
}

eCoinSelectAddress::~eCoinSelectAddress()
{
    delete ui;
}
void eCoinSelectAddress::setWalletModel(WalletModel *_model)
{

    this->model = _model;

    if(_model && _model->getOptionsModel())
    {
        _model->getRecentRequestsTableModel()->sort(RecentRequestsTableModel::Date, Qt::DescendingOrder);
        //connect(_model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        updateDisplayUnit();
        QTableView* tableView = ui->recentRequestsView;
        tableView->verticalHeader()->hide();
        // tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        //tableView->setModel(_model->getRecentRequestsTableModel());
        proxyModel = new QSortFilterProxyModel(this);
        proxyModel->setSourceModel(_model->getAddressTableModel());
        proxyModel->setDynamicSortFilter(true);
        proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setFilterRole(AddressTableModel::TypeRole);
        proxyModel->setFilterFixedString(AddressTableModel::Receive);

        tableView->setModel(proxyModel);
        tableView->setColumnHidden(0,true);
        tableView->setColumnWidth(1,700);
        tableView->setAlternatingRowColors(true);
        tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

        connect(tableView->selectionModel(),
                SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
                SLOT(recentRequestsView_selectionChanged(QItemSelection, QItemSelection)));
    }
}

void eCoinSelectAddress::updateDisplayUnit()
{
}

void eCoinSelectAddress::on_recentRequestsView_doubleClicked(const QModelIndex &index)
{
    sendBackSignal(index.data().toString());
}


void eCoinSelectAddress::recentRequestsView_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{

}

// We override the virtual resizeEvent of the QWidget to adjust tables column
// sizes as the tables width is proportional to the dialogs width.
void eCoinSelectAddress::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void eCoinSelectAddress::on_newAdd_pressed()
{
    QString a,b;
    setRandString(a);
    setRandString(b);
   // QString newadd = model->getAddressTableModel()->addRow( AddressTableModel::Receive,a,b);
    QString newadd = model->getAddressTableModel()->addRow( AddressTableModel::Receive,"","");

    sendBackSignal(newadd);
}
void eCoinSelectAddress::setRandString(QString & randString){
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
void eCoinSelectAddress::sendBackSignal(QString address)
{
    if(m_clientType == 0){
        Q_EMIT back0(address);
    }else if(m_clientType == 1){
        Q_EMIT back1(address);
    }else{
        Q_EMIT back(address);
    }

}
QString eCoinSelectAddress::getOldAddress()
{
    QAbstractItemModel *model = ui->recentRequestsView->model ();
    QModelIndex index = model->index(0,1);
    QVariant data = model->data(index);
    QString str = data.toString();
    std::cout<<str.toStdString()<<std::endl;
    return str;
}
