
#include "MyLabel.h"

EventLabel::EventLabel(QWidget *widget)
: QLabel(widget)
{
   str1 = this->text();
}

EventLabel::~EventLabel(){
    disconnect();
}
void EventLabel::focusInEvent(QFocusEvent *ev)
{

}

void EventLabel::focusOutEvent(QFocusEvent *ev)
{
}


