/*
 * @file CDateEdit.h
 * @brief CDateEdit 是编辑日期的类
 * @date: 2017-07-05
 * @author: liuyan
 */
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
    /**
             * @brief 时间选择构造函数
             * @param parent是父类指针
             */
    CCalendarWidget(QWidget *parent = 0);
    ~CCalendarWidget();

    /**
             * @brief 设置选择的日期
             * @param  date设置的日期
             * @return 无
             */
    void setSelectedDate(const QDate &date);

    /**
             * @brief 获取选择的日期
             * @param  无
             * @return 选择的日期
             */
    QDate selectedDate();

    void setSize(int basew, int baseh);

Q_SIGNALS:
    /**
             * @brief 发送双击选择日期信号
             * @param date选择的日期
             * @return 无
             */
    void activated(const QDate &date);

    /**
             * @brief 发送单击选择日期信号
             * @param date选择的日期
             * @return 无
             */
    void clicked(const QDate &date);

private Q_SLOTS:
    /**
             * @brief 接收双击选择日期事件
             * @param date选择的日期
             * @return 无
             */
    void recvActivated(const QDate &date);

    /**
             * @brief 接收单击选择日期事件
             * @param date选择的日期
             * @return 无
             */
    void recvClicked(const QDate &date);

    /**
             * @brief 前一年按钮点击事件
             * @param 无
             * @return 无
             */
    void yearGoBtn_clicked();

    /**
             * @brief 前一月按钮点击事件
             * @param 无
             * @return 无
             */
    void monthGoBtn_clicked();

    /**
             * @brief 后一月按钮点击事件
             * @param 无
             * @return 无
             */
    void monthBackBtn_clicked();

    /**
             * @brief 后一年按钮点击事件
             * @param 无
             * @return 无
             */
    void yearBackBtn_clicked();

protected:
    /**
             * @brief 窗口显示事件
             * @param  event是事件指针
             * @return 无
             */
    void showEvent(QShowEvent *event);

private:
    QWidget         *m_controlWidget;
    QCalendarWidget *m_calendarWidget;
};

class CDateEdit : public QLineEdit
{
    Q_OBJECT
public:
    /**
             * @brief 日期编辑构造函数
             * @param parent是父类指针
             */
    CDateEdit(QWidget *parent = 0);
    ~CDateEdit();

    /**
             * @brief 设置日期
             * @param date设置的日期
             * @return 无
             */
    void setDate(QDate date);

    /**
             * @brief 获取日期时间
             * @param 无
             * @return 获取的日期时间
             */
    QDate date();

    /**
             * @brief 设置日期格式
             * @param date设置的日期格式
             * @return 无
             */
    void setDateTextFormat(QString format);

    /**
             * @brief 获取日期格式
             * @param 无
             * @return 日期格式
             */
    QString dateTextFormat();

    /**
             * @brief 设置时间控件位置
             * @param 无
             * @return 无
             */
    void setCalendarWidgetPos();
private Q_SLOTS:
    /**
             * @brief 接收文本变化消息
             * @param text变化文本
             * @return 无
             */
    void recvTextChanged(const QString &text);

    /**
             * @brief 接收回车键事件
             * @param 无
             * @return 无
             */
    void recvReturnPressed();

    /**
             * @brief 接收选择文本事件
             * @param 无
             * @return 无
             */
    void recvSelectionChanged();

    /**
             * @brief 接收双击选择日期事件
             * @param date选择的日期
             * @return 无
             */
    void recvActivated(const QDate &date);

    /**
             * @brief 接收单击选择日期事件
             * @param date选择的日期
             * @return 无
             */
    void recvClicked(const QDate &date);

    /**
             * @brief 图标按钮点击事件
             * @param 无
             * @return 无
             */
    void iconButton_clicked();
protected:
    /**
             * @brief 鼠标按下事件消息处理
             * @param  event鼠标按下事件
             * @return 无
             */
    void mousePressEvent(QMouseEvent *event);

    /**
             * @brief 鼠标释放事件消息处理
             * @param  event鼠标释放事件
             * @return 无
             */
    void mouseReleaseEvent(QMouseEvent *event);

    /**
             * @brief 大小变化处理
             * @param  event大小变化事件
             * @return 无
             */
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
