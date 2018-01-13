#ifndef CDATEEDIT_H
#define CDATEEDIT_H

#include <QDate>
#include <QLineEdit>
#include <QDialog>
#include <QLabel>
QT_BEGIN_NAMESPACE
class QCalendarWidget;
QT_END_NAMESPACE
class CCalendarWidget : public QDialog
{
    Q_OBJECT
public:

    CCalendarWidget(QWidget *parent = 0);
    ~CCalendarWidget();

    void setSelectedDate(const QDate &date);


    QDate selectedDate();
    void setMinimumDate(QDate &date);
    void setSize(int basew, int baseh);

Q_SIGNALS:

    void activated(const QDate &date);
    void clicked(const QDate &date);

private Q_SLOTS:

    void recvActivated(const QDate &date);
    void recvClicked(const QDate &date);
    void yearGoBtn_clicked();
    void monthGoBtn_clicked();
    void monthBackBtn_clicked();
    void yearBackBtn_clicked();

protected:
    void showEvent(QShowEvent *event);

private:
    QWidget         *m_controlWidget;
    QCalendarWidget *m_calendarWidget;
};

class CDateEdit : public QLineEdit
{
    Q_OBJECT
public:
    CDateEdit(QWidget *parent = 0);
    ~CDateEdit();
    void setDate(QDate date);
    QDate date();
    void setDateTextFormat(QString format);
    QString dateTextFormat();
    void setCalendarWidgetPos();

    void setMinimumDate(QDate &date);
private Q_SLOTS:
    void recvTextChanged(const QString &text);
    void recvReturnPressed();
    void recvSelectionChanged();
    void recvActivated(const QDate &date);
    void recvClicked(const QDate &date);
    void iconButton_clicked();
protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    bool        m_isSelect;
    QDate       m_date;
    QString     m_dateTextFormat;
    QValidator *m_validator;
    QPushButton     *m_iconButton;
    CCalendarWidget *m_calendarWidget;

};

#endif // CDATEEDIT_H
