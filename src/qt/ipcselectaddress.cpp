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
        //QMessageBox::information(NULL,"","SS");
        _model->getRecentRequestsTableModel()->sort(RecentRequestsTableModel::Date, Qt::DescendingOrder);
        //connect(_model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        updateDisplayUnit();
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

        //tableView->setModel(_model->getAddressTableModel());
        tableView->setColumnHidden(0,true);
		#if QT_VERSION < 0x050000
			tableView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
	    #else
			tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		#endif
	    
        
        //tableView->setColumnWidth(1,700);
        tableView->setAlternatingRowColors(true);
        tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

        connect(tableView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
            SLOT(recentRequestsView_selectionChanged(QItemSelection, QItemSelection)));
        // Last 2 columns are set by the columnResizingFixer, when the table geometry is ready.
       // columnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(tableView, AMOUNT_MINIMUM_COLUMN_WIDTH, DATE_COLUMN_WIDTH, this);
    }
}

void ipcSelectAddress::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
       // ui->reqAmount->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
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
void ipcSelectAddress::recentRequestsView_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{

}

// We override the virtual resizeEvent of the QWidget to adjust tables column
// sizes as the tables width is proportional to the dialogs width.
void ipcSelectAddress::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    //columnResizingFixer->stretchColumnWidth(RecentRequestsTableModel::Message);
}

