#ifndef ADDBOOKWIDGET_H
#define ADDBOOKWIDGET_H

class AddressTableModel;
class AddModel;
#include <QWidget>
#include<QItemDelegate>
#include <QStandardItemModel>
#include "guiutil.h"//20170807
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
class IconDelegate0 : public QItemDelegate
{
    Q_OBJECT
public:
    IconDelegate0(QObject *parent = 0): QItemDelegate(parent) { }
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex & index ) const;

};
class IconDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    IconDelegate(QObject *parent = 0): QItemDelegate(parent) { }
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex & index ) const;
};
class IconDelegate1 : public QItemDelegate
{
    Q_OBJECT
public:
    IconDelegate1(QObject *parent = 0): QItemDelegate(parent) { }
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex & index ) const;
    // {
    //show.bmp是在工程目录中的一张图片（其实就是QQ的图标啦，呵呵）
    //  QPixmap pixmap = QPixmap(":/res/png/ren.png").scaled(24, 24);
    // qApp->style()->drawItemPixmap(painter, option.rect,  Qt::AlignCenter, QPixmap(pixmap));
    //}
};
class MyStandardItemModel:public QStandardItemModel
{
public:
    MyStandardItemModel(QObject * parent=0)
        :QStandardItemModel(parent){}
    virtual ~ MyStandardItemModel(){}
    QVariant data(const QModelIndex & index,
                  int role=Qt::DisplayRole) const;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role=Qt::DisplayRole) const;
};
class MyItemDelegate:public QItemDelegate
{
public:
    MyItemDelegate(QObject * parent=0);
    virtual ~ MyItemDelegate(){}

    void paint(QPainter * painter,
               const QStyleOptionViewItem & option,
               const QModelIndex & index) const;
    bool editorEvent(QEvent * event,
                     QAbstractItemModel * model,
                     const QStyleOptionViewItem & option,
                     const QModelIndex & index);



private:
    QPixmap favouritePixmap;
    QPixmap notFavouritePixmap;
};
class AddBookWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AddBookWidget(QWidget *parent = 0);
    ~AddBookWidget();
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
    void openeditadddialog(AddressTableModel *atm,int tag);
    void backSend();
    void selectaddressok(QString a);
    void selectaddressyes(QString a,QString b);
    void ecoinpageAddEdit();


void textChanged(QString text);
private Q_SLOTS:
    void on_newButton_pressed();
    void on_backButton_pressed();
    void on_listView_clicked(const QModelIndex &index);
    void on_AddButton_pressed();
    void slotTextChange(const QString &s);
   // void showEditline();

void txChanged(QString t,QString add);


private:
//MyLabel* namelabel ;
QLineEdit* namelabel;
QLabel* timelabel;
    int etag  ;
    Ui::AddBookWidget *ui;
    AddressTableModel *model;
    AddModel *modelmodel;
    QSortFilterProxyModel *proxyModel;
    QSortFilterProxyModel *proxyModelnew;
    QStandardItemModel *modelnew;
    QStandardItemModel *modelnewnew;
    QVBoxLayout * pvboxlayoutall;
    MyItemDelegate * delegate;
    MyStandardItemModel * modeltttt;
    IconDelegate *iconDelegate;
    IconDelegate1 *iconDelegate1;
    IconDelegate0 *iconDelegate0;
    std::unique_ptr<AddressTableModel> filter;
    map<QString,QString>   my_Map;
};

#endif // ADDBOOKWIDGET_H
