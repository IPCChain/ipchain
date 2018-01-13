#ifndef MYSCROLLAREA
#define MYSCROLLAREA
#include <QObject>
#include <QScrollArea>
#include <QPoint>

class MyScrollArea : public QScrollArea
{
    Q_OBJECT

public:
    MyScrollArea(QWidget* parent = NULL);
    ~MyScrollArea();

protected:
    bool eventFilter(QObject *obj,QEvent *evt);

private:
    bool mMoveStart;
    bool mContinuousMove;
    QPoint mMousePoint;
};
#endif // MYSCROLLAREA

