
#include "MyLabel.h"
#include "log/log.h"

Label::Label( QWidget *parent):QLabel(parent)
{
    this->clearFocus();
    mouse_press = false;
    clicked_num = 0;
    timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(SlotTimerOut()));
}

Label::~Label()
{
    delete timer;
}



