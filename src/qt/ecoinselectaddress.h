#ifndef ECOINSELECTADDRESS_H
#define ECOINSELECTADDRESS_H

#include "guiutil.h"
#include <QWidget>
#include <QModelIndex>
#include <QItemSelection>
class QSortFilterProxyModel;
class WalletModel;

#include <QWidget>

namespace Ui {
class eCoinSelectAddress;
}

class eCoinSelectAddress : public QWidget
{
    Q_OBJECT

public:
    explicit eCoinSelectAddress(QWidget *parent = 0);
    ~eCoinSelectAddress();

    WalletModel *model;
    void setWalletModel(WalletModel * _model);
    virtual void resizeEvent(QResizeEvent *event);
    GUIUtil::TableViewLastColumnResizingFixer *columnResizingFixer;
    void setClientType(int type){m_clientType = type;}
    int getClientType(){return m_clientType;}
    QString getOldAddress();
private:
    Ui::eCoinSelectAddress *ui;
    QSortFilterProxyModel *proxyModel;
    int m_clientType;
    enum ColumnWidths {
        DATE_COLUMN_WIDTH = 130,
        LABEL_COLUMN_WIDTH = 120,
        AMOUNT_MINIMUM_COLUMN_WIDTH = 180,
        MINIMUM_COLUMN_WIDTH = 130
    };
   void setRandString(QString & randString);
   void sendBackSignal(QString);
Q_SIGNALS:
    void back(QString);//other
    void back0(QString);//register
    void back1(QString);//selecetrecive

private Q_SLOTS:
    void on_recentRequestsView_doubleClicked(const QModelIndex &index);
    void recentRequestsView_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void updateDisplayUnit();
    void on_newAdd_pressed();
};

#endif // ECOINSELECTADDRESS_H
