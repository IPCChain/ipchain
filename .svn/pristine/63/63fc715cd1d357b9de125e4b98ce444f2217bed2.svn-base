#include "addbookwidget.h"
#include "forms/ui_addbookwidget.h"
#include "addresstablemodel.h"
#include "ui_interface.h"
#include "clickqlabel.h"
#include <QLineEdit>
#include "addmodel.h"
#include <QHBoxLayout>
#include "upgradewidget.h"
#include <QSortFilterProxyModel>
#include <QAbstractItemDelegate>
#include <QPainter>
#include <QMenu>
#include <guiutil.h>
#include <QAbstractItemDelegate>
#include <QScrollBar>
#include <QMouseEvent>
#include <QPixmapCache>
bool AddBookWidget::eventFilter(QObject *obj, QEvent *event)
{

    QMouseEvent *ev = (QMouseEvent*)event;//QEvent::KeyPress
    if (obj == ui->scrollArea) {
        if (ev->button() ==Qt::LeftButton || event->type() == QEvent::MouseMove) {
            QMouseEvent *event = static_cast<QMouseEvent*> (event);
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
    else if(obj == timelabel)
    {
        if (event->type() == QEvent::KeyRelease)
        {
            return true;
        } else {
            return false;
        }
    }
    else {
    }
}
AddBookWidget::AddBookWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AddBookWidget)
{
    ui->setupUi(this);
    ui->scrollArea->installEventFilter(this);
    QWidget * pnewall = new QWidget();
    pvboxlayoutall = new QVBoxLayout();
    pnewall->setLayout(pvboxlayoutall);

    ui->scrollArea->setWidget(pnewall);
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
    if(TAB_set == gettag())
    {
        pnewone->setIntertag(5);
    }
    else if(TAB_other == gettag())
    {
        pnewone->setIntertag(3);
    }
    else if(TAB_send == gettag())
    {
        pnewone->setIntertag(2);
    }
    QHBoxLayout * phboxlayout = new QHBoxLayout();
    pnewone->setLayout(phboxlayout);
    static int num=0;
    num++;
    QLabel* pictypelabel = new QLabel(QString::number(num));
    phboxlayout->addWidget(pictypelabel);//ren
    QVBoxLayout * pvboxlayout = new QVBoxLayout();
    phboxlayout->addLayout(pvboxlayout);

    if(TAB_send == gettag())
    {
        namelabel0 = new ClickQLabel();
        namelabel0->setText(name);
        QFont font("Microsoft YaHei", 12, 35);
        namelabel0->setFont(font);
        pvboxlayout->addWidget(namelabel0);
    }
    else
    {
        namelabel =new QLineEdit();
        namelabel->setFocusPolicy(Qt::ClickFocus);
        namelabel->setStyleSheet("background-color:rgb(242,241,240);border:0px");//color:#F2F1F0;
        namelabel->setMouseTracking(true);
        namelabel->setAutoFillBackground(true);
        namelabel->setText(name);

        QFont ftname;
        ftname.setPointSize(12);
        QFont font("Microsoft YaHei", 12, 35);
        namelabel->setFont(font);//"Timers" , 28 ,  QFont::Bold)
        QPalette paname;
        paname.setColor(QPalette::WindowText,Qt::gray);
        namelabel->setPalette(paname);
        connect(namelabel,SIGNAL(textChanged(QString)),pnewone,SLOT(changetext(QString)));
        pvboxlayout->addWidget(namelabel);
    }
    connect(pnewone,SIGNAL(updateinfo(QString,QString)),this,SLOT(txChanged(QString,QString)));

    if(TAB_send == gettag())
    {
        timelabel = new ClickQLabel();
        timelabel->setText(add);
        QFont fttime("Microsoft YaHei", 12, 35);;
        timelabel->setFont(fttime);
        QPalette patime;
        patime.setColor(QPalette::WindowText,Qt::gray);
        timelabel->setPalette(patime);
        pvboxlayout->addWidget(timelabel);
    }
    else
    {
        timelabel0 =new QLineEdit();
        timelabel0->setReadOnly(true);
        timelabel0->setFocusPolicy(Qt::ClickFocus);
        timelabel0->setStyleSheet("background-color:rgb(242,241,240);border:0px");//color:#F2F1F0;
        timelabel0->setMouseTracking(true);
        timelabel0->setAutoFillBackground(true);
        timelabel0->setText(add);

        QFont font("Microsoft YaHei", 9, 35);
        timelabel0->setFont(font);//"Timers" , 28 ,  QFont::Bold)
        QPalette paname;
        paname.setColor(QPalette::WindowText,Qt::gray);
        pvboxlayout->addWidget(timelabel0);
    }
    pnewone->setadd(add);
    pnewone->setlabel(name);

    pvboxlayoutall->addWidget(pnewone);
    connect(pnewone,SIGNAL(pressw(QString,QString)),this,SIGNAL(selectaddressyes(QString,QString)) );

    QString picpath;
    QString picprogesspath;

    picprogesspath =":/res/png/ren.png";
    pictypelabel->setFixedSize(57,57);
    QPixmapCache::clear();
    QPixmapCache::setCacheLimit(1);
    QPixmap p;
    p.load(picprogesspath);
    pictypelabel->setPixmap(p);

    txchangmap txmap;
    txmap.txadd =add;
    txmap.txlabel = name;
    return 1;

}


void AddBookWidget::txChanged(QString t,QString add)
{
    this->model->updateEntry(add,t,false,"",ChangeType::CT_UPDATEDM);
}

void AddBookWidget::settag(int tag)
{
    etag = tag;

    if(TAB_set == etag)
    {
        ui->backButton->setVisible(false);
    }
    else if(TAB_send == etag)
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
    int t = proxyModel->rowCount();
    for(int i= 0,j=0;i<t*2;i+=2,j++)
    {
        QModelIndex topindex = proxyModel->index(j,0, QModelIndex());
        QModelIndex buttomindex=  proxyModel->index(j,1, QModelIndex());
        QVariant topvar= topindex.data();
        QVariant bumvar= buttomindex.data();
        addnewoneipcmessage(1,topvar.toString(),bumvar.toString(),1);
    }

    QSpacerItem* horizontalSpacer = new QSpacerItem(20,40,QSizePolicy::Minimum, QSizePolicy::Expanding);
    pvboxlayoutall->addSpacerItem(horizontalSpacer);
}
AddBookWidget::~AddBookWidget()
{
    delete ui;
}

void AddBookWidget::on_backButton_pressed()
{
    Q_EMIT backSend();
}

