#ifndef ADDBOOKWIDGET_H
#define ADDBOOKWIDGET_H

class AddressTableModel;
class AddModel;
#include <QWidget>
#include<QItemDelegate>
#include <QStandardItemModel>
#include "guiutil.h"
#include <QWidget>
#include <iostream>
#include <QVBoxLayout>
#include "MyLabel.h"
using namespace std;

QT_BEGIN_NAMESPACE
class QItemSelection;
class QMenu;
class QModelIndex;
class QSortFilterProxyModel;
class QTableView;
class MyItemDelegate;
class MyStandardItemModel;

QT_END_NAMESPACE
namespace Ui {
class AddBookWidget;
}
;

struct txchangmap{
    QString txadd;
    QString txlabel;
};
class AddBookWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AddBookWidget(QWidget *parent = 0);
    ~AddBookWidget();
    enum TabTypes {
        TAB_send = 2,
        TAB_set = 5,
        TAB_other = 3
    };
    QString returnaddress;
    int addnewoneipcmessage(int pictype,QString name,QString add,int picprogess);
    bool  AddEditedRow(QString key,QString value);
    void setModel(AddressTableModel *_model);
    void selectNewAddress(const QModelIndex &parent, int begin, int /*end*/);
    //void setModel(WalletModel *model);
    QStandardItemModel *student_model;

    bool eventFilter(QObject *obj, QEvent *event);
    void settag(int tag);

    int gettag();

Q_SIGNALS:
    void backSend();
    void selectaddressyes(QString a,QString b);
    void textChanged(QString text);
private Q_SLOTS:
    void on_backButton_pressed();
    void txChanged(QString t,QString add);


private:
    QLineEdit* namelabel ;
    QLabel* namelabel0 ;
    QLabel* timelabel ;
    QLineEdit* timelabel0;
    int etag ;
    Ui::AddBookWidget *ui;
    AddressTableModel *model;
    AddModel *modelmodel;
    QSortFilterProxyModel *proxyModel;
    QSortFilterProxyModel *proxyModelnew;
    QVBoxLayout * pvboxlayoutall;
    std::unique_ptr<AddressTableModel> filter;
    map<QString,QString>   my_Map;
};

#endif // ADDBOOKWIDGET_H
