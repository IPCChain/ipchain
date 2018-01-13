#ifndef QTHYPERLINK
#define QTHYPERLINK
#include <QObject>
#include <QLabel>
#include <QPoint>

class MyQTHyperLink : public QLabel
{
    Q_OBJECT

public:
    MyQTHyperLink(QWidget* parent = NULL);
    ~MyQTHyperLink();

Q_SIGNALS:
    void clicked(MyQTHyperLink* click);
protected:
    void mouseReleaseEvent(QMouseEvent *);
private:
    bool mMoveStart;
    bool mContinuousMove;
    QPoint mMousePoint;
};
#endif // QTHYPERLINK

