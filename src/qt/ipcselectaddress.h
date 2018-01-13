#ifndef IPCSELECTADDRESS_H
#define IPCSELECTADDRESS_H
#include "guiutil.h"
#include <QWidget>
#include <QModelIndex>
#include <QItemSelection>
class QSortFilterProxyModel;
class WalletModel;
namespace Ui {
class ipcSelectAddress;
}

class ipcSelectAddress : public QWidget
{
    Q_OBJECT

public:
    explicit ipcSelectAddress(QWidget *parent = 0);
    ~ipcSelectAddress();

    void setTag(int e);
    int getTag();
    WalletModel *model;
    void setWalletModel(WalletModel * _model);
    virtual void resizeEvent(QResizeEvent *event);
    GUIUtil::TableViewLastColumnResizingFixer *columnResizingFixer;
private:
    Ui::ipcSelectAddress *ui;
    QSortFilterProxyModel *proxyModel;
    int m_tag;
    enum ColumnWidths {
        DATE_COLUMN_WIDTH = 130,
        LABEL_COLUMN_WIDTH = 120,
        AMOUNT_MINIMUM_COLUMN_WIDTH = 180,
        MINIMUM_COLUMN_WIDTH = 130
    };

Q_SIGNALS:
    void back(QString);
    void back3(QString,int);
private Q_SLOTS:
    void on_recentRequestsView_doubleClicked(const QModelIndex &index);


};

#endif // IPCSELECTADDRESS_H
