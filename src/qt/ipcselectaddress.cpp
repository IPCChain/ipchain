#include "ipcselectaddress.h"
#include "ui_ipcselectaddress.h"
#include "walletmodel.h"
#include <QTableView>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include "guiutil.h"
#include "recentrequeststablemodel.h"
#include "addresstablemodel.h"
ipcSelectAddress::ipcSelectAddress(QWidget *parent) :
    QWidget(parent),model(NULL),
    ui(new Ui::ipcSelectAddress)
{
    ui->setupUi(this);
}

ipcSelectAddress::~ipcSelectAddress()
{
    delete ui;
}
void ipcSelectAddress::setWalletModel(WalletModel *_model)
{
    this->model = _model;
    if(_model && _model->getOptionsModel())
    {
        _model->getRecentRequestsTableModel()->sort(RecentRequestsTableModel::Date, Qt::DescendingOrder);
        QTableView* tableView = ui->recentRequestsView;
        tableView->verticalHeader()->hide();
        proxyModel = new QSortFilterProxyModel(this);
        proxyModel->setSourceModel(_model->getAddressTableModel());
        proxyModel->setDynamicSortFilter(true);
        proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setFilterRole(AddressTableModel::TypeRole);
        proxyModel->setFilterFixedString(AddressTableModel::Receive);
        tableView->setModel(proxyModel);
        tableView->setColumnHidden(0,true);
		#if QT_VERSION < 0x050000
			tableView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
	    #else
			tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		#endif
        tableView->setAlternatingRowColors(true);
        tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

     }
}
void ipcSelectAddress::on_recentRequestsView_doubleClicked(const QModelIndex &index)
{
    if(3 == m_tag)
    {
        Q_EMIT back3(index.data().toString(),3);
    }
    else
    {
    Q_EMIT back(index.data().toString());
    }
}

void ipcSelectAddress::setTag(int e)
{
    m_tag = e;
}
int ipcSelectAddress::getTag()
{
    return m_tag;
}
void ipcSelectAddress::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

