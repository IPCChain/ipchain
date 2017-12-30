#ifndef CLICKQLABEL_H
#define CLICKQLABEL_H
#include <QLabel>
#include <QEvent>
class ClickQLabel : public QLabel
{

    Q_OBJECT
public:
    ClickQLabel(QWidget * parent = 0);

    ClickQLabel(const QString &text, QWidget *parent = 0);

    virtual void mouseReleaseEvent(QMouseEvent * ev);
Q_SIGNALS:
    void clicked(void);
    void Labelchange();


};

#endif // CLICKQLABEL_H
