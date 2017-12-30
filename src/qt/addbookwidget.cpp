#include "addbookwidget.h"
#include "forms/ui_addbookwidget.h"
#include "addresstablemodel.h"
#include "ui_interface.h"
#include "clickqlabel.h"
#include <QLineEdit>

#include "addmodel.h"
#include <QHBoxLayout>
#include "upgradewidget.h"
#include "editaddressdialog.h"
#include "editadddialog.h"
#include "addfilterproxy.h"
#include <QSortFilterProxyModel>
//#include <QSqlQueryModel>
#include <QMessageBox>
#include <qpainter.h>>

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QMenu>
#include <guiutil.h>
#include <QAbstractItemDelegate>
#include <QScrollBar>
#include <QMouseEvent>
#include <QPixmapCache>
QVariant MyStandardItemModel::data(
        const QModelIndex & index,
        int role) const
{

}

QVariant MyStandardItemModel::headerData(int section,
                                         Qt::Orientation orientation,
                                         int role) const
{

}

void IconDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex & index ) const
{
    QPixmap pixmap = QPixmap(":/res/png/ren.png").scaled(24, 24);
    qApp->style()->drawItemPixmap(painter, option.rect,  Qt::AlignCenter, QPixmap(pixmap));
}

void IconDelegate1::paint(QPainter *painter, const QStyleOptionViewItem &option,
                          const QModelIndex & index ) const
{
    painter->setPen(QColor(139, 139, 139));
}

void IconDelegate0::paint(QPainter *painter, const QStyleOptionViewItem &option,
                          const QModelIndex & index ) const
{

    QVariant value = index.data(0);

    QString x = value.toString();
    QString y = "\n";
    int t=x.indexOf(y);

    QString label = x.mid(0, t);
    QString add = x.mid(t);

    QRect mainRect = option.rect;
    QRect labelRect(mainRect.left() , mainRect.top(), mainRect.width(), 25);
    QRect addRect(mainRect.left() , mainRect.top(), mainRect.width(), 25);


    QLabel *label_text  = new QLabel();
    label_text->setText("11111111111");
    label_text->setGeometry(mainRect.left(), mainRect.top(),mainRect.width(), mainRect.top()+25);

    painter->setPen(QColor(139, 139, 139));

    painter->setClipping(true);
}

MyItemDelegate::MyItemDelegate(QObject * parent)
    :QItemDelegate(parent)
{
    favouritePixmap=QPixmap(":/icons/add");
    notFavouritePixmap=QPixmap(":/icons/add");
}

void MyItemDelegate::paint(QPainter * painter,
                           const QStyleOptionViewItem & option,
                           const QModelIndex & index) const
{

    painter->save();
    QVariant var= index.data(AddressTableModel::Label);
    QVariant var1= index.data(AddressTableModel::Address);


    int xspace = 55 + 8;
    int ypad = 6;
    QRect mainRect = option.rect;
    QRect decorationRect(mainRect.left()+12,mainRect.top(), 55, 55);

    int halfheight = (mainRect.height() - 2*ypad)/2;
    QRect addRect(mainRect.left(), mainRect.bottom()-20, 375- xspace, halfheight);
    QRect labelRect(mainRect.left()+100 , mainRect.bottom()-20, 375 - xspace, halfheight);


    if(var.isNull()) var=false;
    const QPixmap & star=var.toBool()?
                favouritePixmap:notFavouritePixmap;

    int width=star.width();
    int height=star.height();
    QRect rect=option.rect;
    int x=rect.x()+rect.width()/2-width/2;
    int y=rect.y()+rect.height()/2-height/2;

    painter->drawText(addRect, Qt::AlignLeft|Qt::AlignTop,var1.toString());
    painter->restore();

}

bool MyItemDelegate::editorEvent(QEvent * event,
                                 QAbstractItemModel * model,
                                 const QStyleOptionViewItem & /*option*/,
                                 const QModelIndex & index)
{

}


bool AddBookWidget::eventFilter(QObject *obj, QEvent *event)
{

QMouseEvent *ev = (QMouseEvent*)event;//QEvent::KeyPress



    if (obj == ui->scrollArea) {
        if (ev->button() ==Qt::LeftButton || event->type() == QEvent::MouseMove) {
            QMouseEvent *event = static_cast<QMouseEvent*> (event);
            // QString("Move: %1, %2").arg(QString::number(event->x()), QString::number(event->y()));
            return true;
        } else {
            return false;
        }
    }

    else if(obj == namelabel)
    {

        if (event->type() == QEvent::KeyRelease)
        {
            return true;
        } else {
            return false;
        }


    }

/*
    else if(obj ==timelabel )
    {
QMouseEvent *ev = (QMouseEvent*)event;
       // if (event->type() == QEvent::)
        if ( ev->button() == Qt::RightButton)
        {
            return true;
        } else {
            return false;
        }


    }
*/
    else {
    }
}
AddBookWidget::AddBookWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AddBookWidget)

{
    ui->setupUi(this);
    ui->scrollArea->installEventFilter(this);

    delegate=new MyItemDelegate;
    modelnewnew = new QStandardItemModel(this);

    QWidget * pnewall = new QWidget();
    pvboxlayoutall = new QVBoxLayout();
    pnewall->setLayout(pvboxlayoutall);

    ui->scrollArea->setWidget(pnewall);

    student_model = new QStandardItemModel();

}
bool AddBookWidget::AddEditedRow(QString key,QString value){

    QWidget * pnewall = new QWidget();
    pvboxlayoutall = new QVBoxLayout();
    pnewall->setLayout(pvboxlayoutall);

    ui->scrollArea->setWidget(pnewall);

    model->addRow( AddressTableModel::Send,
                   key,
                   value);
    setModel(model);


    return true;
}

int AddBookWidget::addnewoneipcmessage(int pictype,QString name,QString add,int picprogess)
{
    upgradewidget * pnewone = new upgradewidget();
    pnewone->setMaximumHeight(105);
    pnewone->setMinimumHeight(105);
    if(5 == gettag())
    {
        pnewone->setIntertag(5);

    }
    else if(3 == gettag())
    {
        pnewone->setIntertag(3);

    }
    else if(2 == gettag())
    {
        pnewone->setIntertag(2);

    }
    else
    {
    }
    QHBoxLayout * phboxlayout = new QHBoxLayout();
    pnewone->setLayout(phboxlayout);
    static int num=0;
    num++;
    QLabel* pictypelabel = new QLabel(QString::number(num));

    phboxlayout->addWidget(pictypelabel);//ren

    QVBoxLayout * pvboxlayout = new QVBoxLayout();
    phboxlayout->addLayout(pvboxlayout);
    //QLineEdit*
    namelabel =new QLineEdit();

    namelabel->setStyleSheet("background-color:rgb(242,241,240);border:0px");//color:#F2F1F0;
    namelabel->setMouseTracking(true);
    namelabel->setAutoFillBackground(true);
    namelabel->setText(name);
    QFont ftname;
    ftname.setPointSize(14);

    QFont font("Microsoft YaHei", 14, 35);
    //QFont font = GUIUtil::fixedPitchFont();
    //font.setPixelSize(18);
    namelabel->setFont(font);//"Timers" , 28 ,  QFont::Bold)
    QPalette paname;
    paname.setColor(QPalette::WindowText,Qt::gray);
    namelabel->setPalette(paname);
    connect(namelabel,SIGNAL(textChanged(QString)),pnewone,SLOT(changetext(QString)));
    connect(pnewone,SIGNAL(updateinfo(QString,QString)),this,SLOT(txChanged(QString,QString)));
    pvboxlayout->addWidget(namelabel);

    timelabel = new QLabel(add);


    timelabel->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard
                                       | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByMouse
                                       );

    QFont fttime("Microsoft YaHei", 14, 35);;
    //  fttime.setPointSize(14);
    timelabel->setFont(fttime);
    QPalette patime;
    patime.setColor(QPalette::WindowText,Qt::gray);
    timelabel->setPalette(patime);
    pnewone->setadd(add);
    pnewone->setlabel(name);
    pvboxlayout->addWidget(timelabel);

    pvboxlayoutall->addWidget(pnewone);
    connect(pnewone,SIGNAL(pressw(QString,QString)),this,SIGNAL(selectaddressyes(QString,QString)) );

    connect(pnewone,SIGNAL(presswq(QString)),this,SIGNAL(selectaddressok(QString)) );
    QString picpath;
    QString picprogesspath;


    picprogesspath =":/res/png/ren.png";
    pictypelabel->setFixedSize(57,57);





    QPixmapCache::clear();

QPixmapCache::setCacheLimit(1);
QPixmap p;

p.load(picprogesspath);
pictypelabel->setPixmap(p);



 //   pictypelabel->setPixmap(QPixmap(picprogesspath));
    txchangmap txmap;
    txmap.txadd =add;
    txmap.txlabel = name;
    return 1;

}

void AddBookWidget::txChanged(QString t,QString add)
{


    this->model->updateEntry(add,t,false,"",ChangeType::CT_UPDATEDM);

}
void AddBookWidget::slotTextChange(const QString &s)
{

}
void AddBookWidget::settag(int tag)
{
    etag = tag;

    if(5 == etag)
    {
       ui->backButton->setVisible(false);
    }
    else if(2 == etag)
    {
        ui->backButton->setVisible(true);
    }
    else
    {
        ui->backButton->setVisible(false);
    }

}

int AddBookWidget::gettag()
{
    return etag;
}
void AddBookWidget::setModel(AddressTableModel *_model)
{
    this->model = _model;
    if(!_model)
        return;
    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(_model);

    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    proxyModel->setFilterRole(AddressTableModel::TypeRole);
    proxyModel->setFilterFixedString(AddressTableModel::Send);

    proxyModel->setFilterRole(AddressTableModel::TypeRole);
    proxyModelnew = new QSortFilterProxyModel(this);
    int t = proxyModel->rowCount();

    modeltttt=new MyStandardItemModel;
    modeltttt->setRowCount(6);
    modeltttt->setColumnCount(8);
    modelnew = new QStandardItemModel(this);
    for(int i= 0,j=0;i<t*2;i+=2,j++)
    {

        QModelIndex topindex = proxyModel->index(j,0, QModelIndex());//遍历第一行的所有列
        QModelIndex buttomindex=  proxyModel->index(j,1, QModelIndex());

        QVariant topvar= topindex.data();
        QVariant bumvar= buttomindex.data();

        addnewoneipcmessage(1,topvar.toString(),bumvar.toString(),1);
    }

    for(int i= 0,j=0;i<t*2;i+=2,j++)
    {

        QModelIndex topindex = proxyModel->index(j,0, QModelIndex());//遍历第一行的所有列
        QModelIndex buttomindex=  proxyModel->index(j,1, QModelIndex());

        QVariant topvar= topindex.data();
        QVariant bumvar= buttomindex.data();
        my_Map.insert(pair<QString,QString>(topvar.toString(),bumvar.toString()));
        modelnew->setItem(i,1,new QStandardItem(topvar.toString()));//+"\n"+bumvar.toString()));
        //设置字符颜色

        modelnew->item(i,1)->setForeground(QBrush(QColor(255, 0, 0)));
        //设置字符位置

        modelnew->item(i,1)->setTextAlignment(Qt::AlignLeft);

        modelnew->setItem(i+1,1,new QStandardItem(bumvar.toString()));
        modelnew->item(i+1,1)->setTextAlignment(Qt::AlignLeft);

        modelnewnew->setItem(i,0,new QStandardItem(topvar.toString()+"\n"+bumvar.toString()));
        //设置字符颜色

        modelnewnew->item(i,0)->setForeground(QBrush(QColor(255, 0, 0)));
        //设置字符位置

        modelnewnew->item(i,0)->setTextAlignment(Qt::AlignLeft);
    }

    iconDelegate0 = new IconDelegate0();
    connect(_model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewAddress(QModelIndex,int,int)));

    QSpacerItem* horizontalSpacer = new QSpacerItem(20,40,QSizePolicy::Minimum, QSizePolicy::Expanding);
    pvboxlayoutall->addSpacerItem(horizontalSpacer);
}
AddBookWidget::~AddBookWidget()
{
    delete ui;
}
void AddBookWidget::on_newButton_pressed()
{
    if(!model)
        return;
    int a =gettag();
    Q_EMIT openeditadddialog(model,a);

}

void AddBookWidget::on_backButton_pressed()
{
    Q_EMIT backSend();

}

void AddBookWidget::selectNewAddress(const QModelIndex &parent, int begin, int /*end*/)
{

    QModelIndex idx = proxyModel->mapFromSource(model->index(begin, AddressTableModel::Address, parent));
    if(idx.isValid() && (idx.data(Qt::EditRole).toString() =="NNNNNNN"))// newAddressToSelect))
    {

    }
}
void AddBookWidget::on_listView_clicked(const QModelIndex &index)
{

}

void AddBookWidget::on_AddButton_pressed()
{

    Q_EMIT ecoinpageAddEdit();
}


