#include "clickqlabel.h"
#include <QMessageBox>
ClickQLabel::ClickQLabel(QWidget* parent):QLabel(parent)
{

}
ClickQLabel::ClickQLabel(const QString &text, QWidget *parent )
{

    this->setText(text);
}

void ClickQLabel::mouseReleaseEvent(QMouseEvent * ev)
{
   Q_UNUSED(ev);
   Q_EMIT clicked();
}
