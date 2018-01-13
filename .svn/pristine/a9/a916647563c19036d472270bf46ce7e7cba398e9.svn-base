#include "qthyperlink.h"
#include <QMouseEvent>
#include <QLabel>
#include <QMessageBox>
MyQTHyperLink::MyQTHyperLink(QWidget *parent)
    :QLabel(parent)
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

void MyQTHyperLink::mouseReleaseEvent(QMouseEvent *)
{
    Q_EMIT clicked(this);
}
