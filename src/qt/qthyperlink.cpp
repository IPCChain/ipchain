
#include "qthyperlink.h"
#include <QMouseEvent>
#include <QLabel>
#include <QMessageBox>
//#include "myscrollbar.h"

MyQTHyperLink::MyQTHyperLink(QWidget *parent)
    :QLabel(parent)     //基类
    ,mMoveStart(false)
    ,mContinuousMove(false)
    ,mMousePoint(QPoint(0,0))
{
    installEventFilter(this);

    this->setMouseTracking(true);
}

MyQTHyperLink::~MyQTHyperLink()
{
}






//别忘了在构造函数中和析构函数中设置鼠标跟踪属性

void MyQTHyperLink::mouseReleaseEvent(QMouseEvent *)
{
   // int index = this->textCursor().anchor();//得到当前光标点击的位置在文本中的第几个
   // Q_EMIT SignalOpenUrl(index);
    Q_EMIT clicked(this);  // 点击信号
}
/*
connect(this->textEdit,SIGNAL(SignalOpenUrl(int)),this,SLOT(SlotlOpenUrl(int)));//连接

void CCLChatTextItemForm::SlotlOpenUrl(int index)
{
    if(urlIndex.find(index) == this->urlIndex.end() )
        return;
    QString urlString = this->strArray.at(urlIndex[index]).first;
    QDesktopServices::openUrl(QUrl(urlString));
}
*/









/*

bool MyQTHyperLink::eventFilter(QObject *obj, QEvent *evt)
{
    if(evt->type() == QEvent::MouseMove)
    {
        QMouseEvent *me = (QMouseEvent*) evt;
        if(me->buttons() & Qt::LeftButton)
        {
            if(!mMoveStart)
            {
//                if(me->pos().y() < (horizontalScrollBar()->pos().y() + horizontalScrollBar()->height())
//                        || me->pos().x() < (verticalScrollBar()->pos().x() + verticalScrollBar()->width()) )
//                    return false;

                mMoveStart = true;
                mContinuousMove = false;
                mMousePoint = me->globalPos();
            }
            else
            {
//                MyScrollBar *scrollBarx = (MyScrollBar*)horizontalScrollBar();
//                MyScrollBar *scrollBary = (MyScrollBar*)verticalScrollBar();

                QScrollBar *scrollBarx = horizontalScrollBar();
                QScrollBar *scrollBary = verticalScrollBar();

                QPoint p = me->globalPos();
                int offsetx = p.x() - mMousePoint.x();
                int offsety = p.y() - mMousePoint.y();
                if(!mContinuousMove && (offsetx > -10 && offsetx < 10) && (offsety > -10 && offsety < 10))
                    return false;

                mContinuousMove = true;

                scrollBarx->setValue(scrollBarx->value() - offsetx);
                scrollBary->setValue(scrollBary->value() - offsety);
                mMousePoint = p;
            }
            return true;
        }
    }
    else if(evt->type() == QEvent::MouseButtonRelease)
    {
        mMoveStart = false;
    }
    return QObject::eventFilter(obj,evt);
}
*/
