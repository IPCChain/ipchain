#ifndef MyLabel
#define MyLabel
#include <QObject>
#include <QLineEdit>
#include <QLabel>
#include <QPoint>
#include "log/log.h"
#include <QMouseEvent>
#include "QLabel"
#include <QWidget>
#include "qcoreevent.h"
#include "qevent.h"
#include "qtimer.h"

class Label:public QLabel
{
    Q_OBJECT
public:
    explicit Label(QWidget *parent = 0);
    ~Label();

private:
    bool mouse_press;
    int clicked_num;
    QTimer* timer;

Q_SIGNALS:
    void LabelClicked();
    void LabelDoubleClicked();
};


#endif

