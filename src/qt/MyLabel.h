#ifndef MyLabel
#define MyLabel
#include <QObject>
#include <QLineEdit>
#include <QLabel>
#include <QPoint>
#include "addbookwidget.h"
#include "log/log.h"
#include <QMouseEvent>
class AddBookWidget;


class EventLabel : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText USER true)
public:
    EventLabel(QWidget* parent = NULL);
    ~EventLabel();
Q_SIGNALS:
    void Txchange();
protected:
    /*
    void changeEvent(QEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    */
    void focusOutEvent(QFocusEvent *ev);
    void focusInEvent(QFocusEvent *ev);
    QString str1;
};


/*
class LineEditorCreator : public QLineEdit
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText USER true)

public:
    LineEditorCreator(QWidget* parent = NULL);
    ~LineEditorCreator();

    void setText(QString text);
    QString text() const;

    void focusInEvent(QFocusEvent * ev);



    bool  focusNextPrevChild(bool next);


    void  focusOutEvent(QFocusEvent * ev);



//static void setItemWidget(AddBookWidget *w);
Q_SIGNALS:
    void txChange(QString s);

protected:
   // void changeEvent(QEvent *evt);
 //void focusOutEvent(QFocusEvent *event);
   // void inputMethodEvent(QInputMethodEvent *event);
private:
    bool mMoveStart;
    bool mContinuousMove;
    QPoint mMousePoint;

private:
    static AddBookWidget *window;

};
*/
/*
class MyLabel : public QLabel
{
    Q_OBJECT

public:
    MyLabel(QWidget* parent = NULL);
    ~MyLabel();

Q_SIGNALS:
    void txChange(QString s);


protected:
    void changeEvent(QEvent *evt);
 //void focusOutEvent(QFocusEvent *event);
    void inputMethodEvent(QInputMethodEvent *event);
private:
    bool mMoveStart;
    bool mContinuousMove;
    QPoint mMousePoint;
};*/
#endif // MYSCROLLAREA

