#include "CDateEdit.h"
#include <QWidget>
#include <QCalendarWidget>
#include <QDebug>
#include <QMouseEvent>
#include <QPushButton>
#include <QRegExpValidator>
#include <QHBoxLayout>
#include <QToolButton>
        CCalendarWidget::CCalendarWidget(QWidget *parent)
            : QDialog(parent)
        {
            resize(225, 170);
            QHBoxLayout *layout= new QHBoxLayout();
            this->setLayout(layout);
            this->setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
            this->setAutoFillBackground(true);
            this->setMouseTracking(true);
            this->setModal(true);
            this->setWindowTitle(tr("选择日期"));

            m_calendarWidget = new QCalendarWidget(this);
            m_calendarWidget->setMinimumDate(QDate::currentDate());
            m_calendarWidget->setMaximumDate(QDate(3000, 1, 1));
            m_calendarWidget->setGridVisible(true);
            m_calendarWidget->setLocale(QLocale(QLocale::Chinese, QLocale::China));
            m_calendarWidget->setFirstDayOfWeek(Qt::Monday);
            m_calendarWidget->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);//不显示星期数
            QToolButton *pmouthBtn = m_calendarWidget->findChild<QToolButton*>(QLatin1String("qt_calendar_monthbutton"));
            QToolButton *pyearBtn = m_calendarWidget->findChild<QToolButton*>(QLatin1String("qt_calendar_yearbutton"));
            pmouthBtn->setStyleSheet("color: rgb(0, 0, 0);");
            pyearBtn->setStyleSheet("color: rgb(0, 0, 0);");
            connect(m_calendarWidget, SIGNAL(activated(const QDate &)),
                    this, SIGNAL(activated(const QDate &)));
            connect(m_calendarWidget, SIGNAL(clicked(const QDate &)),
                    this, SIGNAL(clicked(const QDate &)));
            layout->addWidget(m_calendarWidget);
            //this->adjustSize();
        }

        CCalendarWidget::~CCalendarWidget()
        {
        }

        void CCalendarWidget::showEvent(QShowEvent *event)
        {
            m_calendarWidget->setFocus();
            QDialog::showEvent(event);
        }

        void CCalendarWidget::recvActivated(const QDate &date)
        {
            Q_EMIT activated(date);
        }

        void CCalendarWidget::recvClicked(const QDate &date)
        {
            Q_EMIT clicked(date);
        }

        void CCalendarWidget::setSelectedDate(const QDate &date)
        {
            m_calendarWidget->setSelectedDate(date);
        }

        QDate CCalendarWidget::selectedDate()
        {
            return m_calendarWidget->selectedDate();
        }

        void CCalendarWidget::yearGoBtn_clicked()
        {
            m_calendarWidget->setSelectedDate(m_calendarWidget->selectedDate().addYears(-1));
             m_calendarWidget->setFocus();
        }

        void CCalendarWidget::monthGoBtn_clicked()
        {
            m_calendarWidget->setSelectedDate(m_calendarWidget->selectedDate().addMonths(-1));
            m_calendarWidget->setFocus();
        }

        void CCalendarWidget::monthBackBtn_clicked()
        {
            m_calendarWidget->setSelectedDate(m_calendarWidget->selectedDate().addMonths(1));
            m_calendarWidget->setFocus();
        }

        void CCalendarWidget::yearBackBtn_clicked()
        {
            m_calendarWidget->setSelectedDate(m_calendarWidget->selectedDate().addYears(1));
            m_calendarWidget->setFocus();
        }

        void CCalendarWidget::setSize(int basew, int baseh)
        {
            setFixedSize(basew,baseh);
        }

        CDateEdit::CDateEdit(QWidget *parent)
            : QLineEdit(parent)
        {
            m_isSelect = false;
            m_dateTextFormat = "yyyy-MM-dd";

            QRegExp regx("(([0-9]{3}[1-9]|[0-9]{2}[1-9][0-9]{1}|[0-9]{1}[1-9][0-9]{2}|[1-9][0-9]{3})-(((0[13578]|1[02])-(0[1-9]|[12][0-9]|3[01]))|((0[469]|11)-(0[1-9]|[12][0-9]|30))|(02-(0[1-9]|[1][0-9]|2[0-8]))))|((([0-9]{2})(0[48]|[2468][048]|[13579][26])|((0[48]|[2468][048]|[3579][26])00))-02-29)");
            m_validator = new QRegExpValidator(regx);
            this->setValidator(m_validator);

            m_iconButton = new QPushButton(this);
            m_iconButton->setFixedSize(this->height()-2, this->height()-2);
            m_iconButton->setFlat(true);
            m_iconButton->setIcon(QIcon(":/images/calendar_icon.png"));
            m_iconButton->setCursor(Qt::PointingHandCursor);
            //m_iconButton->setStyleSheet("QPushButton{background-image: url(:/images/icon.png);border: 0px;}");小图标预留
            m_iconButton->move(this->width() - m_iconButton->width() - 2, 1);
            m_iconButton->hide();
            m_calendarWidget = new CCalendarWidget(this);
            connect(m_calendarWidget, SIGNAL(activated(const QDate &)),
                    this, SLOT(recvActivated(const QDate &)));
            connect(m_calendarWidget, SIGNAL(clicked(const QDate &)),
                    this, SLOT(recvClicked(const QDate &)));

            connect(this, SIGNAL(textChanged(const QString &)),
                    this, SLOT(recvTextChanged(const QString &)));
            connect(this, SIGNAL(returnPressed()),
                    this, SLOT(recvReturnPressed()));
            connect(this, SIGNAL(selectionChanged()),
                    this, SLOT(recvSelectionChanged()));
            connect(m_iconButton, SIGNAL(clicked()),
                    this, SLOT(iconButton_clicked()));
            setContextMenuPolicy(Qt::NoContextMenu);

            setText(m_calendarWidget->selectedDate().toString(m_dateTextFormat));
        }

        CDateEdit::~CDateEdit()
        {
        }

        void CDateEdit::setDateTextFormat(QString format)
        {
            if ("YYYY-MM-DD" == format.toUpper())
            {
                QRegExp regx("(([0-9]{3}[1-9]|[0-9]{2}[1-9][0-9]{1}|[0-9]{1}[1-9][0-9]"
                             "{2}|[1-9][0-9]{3})-(((0[13578]|1[02])-(0[1-9]|[12][0-9]|3"
                             "[01]))|((0[469]|11)-(0[1-9]|[12][0-9]|30))|(02-(0[1-9]|"
                             "[1][0-9]|2[0-8]))))|((([0-9]{2})(0[48]|[2468][048]|[13579]"
                             "[26])|((0[48]|[2468][048]|[3579][26])00))-02-29)");
                delete m_validator;
                m_validator = new QRegExpValidator(regx);
                this->setValidator(m_validator);
                m_dateTextFormat = format;
            }
            else if ("DD/MM/YYYY" == format.toUpper())
            {
                QRegExp regx("(((0[1-9]|[12][0-9]|3[01])/((0[13578]|1[02]))|((0[1-9]|[12]"
                             "[0-9]|30)/(0[469]|11))|(0[1-9]|[1][0-9]|2[0-8])/(02))/([0-9]"
                             "{3}[1-9]|[0-9]{2}[1-9][0-9]{1}|[0-9]{1}[1-9][0-9]{2}|[1-9][0-9]"
                             "{3}))|(29/02/(([0-9]{2})(0[48]|[2468][048]|[13579][26])|((0[48]|"
                             "[2468][048]|[3579][26])00)))");
                delete m_validator;
                m_validator = new QRegExpValidator(regx);
                this->setValidator(m_validator);
                m_dateTextFormat = format;
            }
        }

        QString CDateEdit::dateTextFormat()
        {
            return m_dateTextFormat;
        }

        void CDateEdit::recvActivated(const QDate &date)
        {
            qDebug()<<"CDateEdit::recvActivated"<<date<<endl;
            this->setText(date.toString(m_dateTextFormat));
            m_calendarWidget->hide();
        }

        void CDateEdit::recvClicked(const QDate &date)
        {
            qDebug()<<"CDateEdit::recvClicked"<<date<<endl;
            this->setText(date.toString(m_dateTextFormat));
            m_calendarWidget->hide();
        }

        void CDateEdit::setDate(QDate date)
        {
            m_calendarWidget->setSelectedDate(date);
            this->setText(m_calendarWidget->selectedDate().toString(m_dateTextFormat));
        }

        QDate CDateEdit::date()
        {
            return m_calendarWidget->selectedDate();
        }

        void CDateEdit::iconButton_clicked()
        {
            if (m_calendarWidget->isHidden())
            {
                QRegExp regx;
                if ("YYYY-MM-DD" == m_dateTextFormat.toUpper())
                {
                    regx.setPattern("(([0-9]{3}[1-9]|[0-9]{2}[1-9][0-9]{1}|[0-9]{1}[1-9][0-9]"
                                    "{2}|[1-9][0-9]{3})-(((0[13578]|1[02])-(0[1-9]|[12][0-9]|3"
                                    "[01]))|((0[469]|11)-(0[1-9]|[12][0-9]|30))|(02-(0[1-9]|"
                                    "[1][0-9]|2[0-8]))))|((([0-9]{2})(0[48]|[2468][048]|[13579]"
                                    "[26])|((0[48]|[2468][048]|[3579][26])00))-02-29)");
                }
                else
                {
                    regx.setPattern("(((0[1-9]|[12][0-9]|3[01])/((0[13578]|1[02]))|((0[1-9]|[12]"
                                    "[0-9]|30)/(0[469]|11))|(0[1-9]|[1][0-9]|2[0-8])/(02))/([0-9]"
                                    "{3}[1-9]|[0-9]{2}[1-9][0-9]{1}|[0-9]{1}[1-9][0-9]{2}|[1-9][0-9]"
                                    "{3}))|(29/02/(([0-9]{2})(0[48]|[2468][048]|[13579][26])|((0[48]|"
                                    "[2468][048]|[3579][26])00)))");
                }
                if (regx.exactMatch(this->text()))
                {
                    m_calendarWidget->setSelectedDate(QDate::fromString(this->text(), m_dateTextFormat));
                }
                //m_calendarWidget->move(QCursor::pos()+QPoint(-110, 15));
                setCalendarWidgetPos();
                m_calendarWidget->show();
                m_calendarWidget->raise();
            }
            else
                m_calendarWidget->hide();
        }
        void CDateEdit::setCalendarWidgetPos()
        {
            //QPoint GlobalPoint(mapToGlobal(QPoint(0,0)));
            QPoint winpos = mapToGlobal(QPoint(0,0));
            winpos.setY(winpos.y()+height());
            qDebug()<<height();
            //m_calendarWidget->setSize(170,170);
            //winpos.setX(winpos.x()-pos().x()-(m_calendarWidget->width()-width())/2);
            m_calendarWidget->move(winpos);
            //m_calendarWidget->setSize(width(),200);
           // m_calendarWidget->setFixedSize(width(),200);
        }

        void CDateEdit::resizeEvent(QResizeEvent *event)
        {
            m_iconButton->setFixedSize(this->height()-2, this->height()-2);
            m_iconButton->move(this->width() - m_iconButton->width() - 1, 1);
            this->setTextMargins(0, 0, m_iconButton->width()+1, 0);

            QLineEdit::resizeEvent(event);
        }

        void CDateEdit::mousePressEvent(QMouseEvent *event)
        {
            if(event->button() == Qt::LeftButton)
            {
                m_isSelect = false;
            }

            QLineEdit::mousePressEvent(event);
        }

        void CDateEdit::mouseReleaseEvent(QMouseEvent *event)
        {

            if(event->button() == Qt::LeftButton)
            {
                if (false == m_isSelect)
                {
                    QRegExp regx;
                    if ("YYYY-MM-DD" == m_dateTextFormat.toUpper())
                    {
                        regx.setPattern("(([0-9]{3}[1-9]|[0-9]{2}[1-9][0-9]{1}|[0-9]{1}[1-9][0-9]"
                                        "{2}|[1-9][0-9]{3})-(((0[13578]|1[02])-(0[1-9]|[12][0-9]|3"
                                        "[01]))|((0[469]|11)-(0[1-9]|[12][0-9]|30))|(02-(0[1-9]|"
                                        "[1][0-9]|2[0-8]))))|((([0-9]{2})(0[48]|[2468][048]|[13579]"
                                        "[26])|((0[48]|[2468][048]|[3579][26])00))-02-29)");
                    }
                    else
                    {
                        regx.setPattern("(((0[1-9]|[12][0-9]|3[01])/((0[13578]|1[02]))|((0[1-9]|[12]"
                                        "[0-9]|30)/(0[469]|11))|(0[1-9]|[1][0-9]|2[0-8])/(02))/([0-9]"
                                        "{3}[1-9]|[0-9]{2}[1-9][0-9]{1}|[0-9]{1}[1-9][0-9]{2}|[1-9][0-9]"
                                        "{3}))|(29/02/(([0-9]{2})(0[48]|[2468][048]|[13579][26])|((0[48]|"
                                        "[2468][048]|[3579][26])00)))");
                    }
                    if (regx.exactMatch(this->text()))
                    {
                        m_calendarWidget->setSelectedDate(QDate::fromString(this->text(), m_dateTextFormat));
                    }
                    //m_calendarWidget->move(QCursor::pos()+QPoint(-110, 15));
                    setCalendarWidgetPos();
                    m_calendarWidget->show();
                    m_calendarWidget->raise();
                }
            }

            QLineEdit::mouseReleaseEvent(event);
        }

        void CDateEdit::recvTextChanged(const QString &text)
        {
            qDebug()<<"CDateEdit::recvTextChanged"<<text<<endl;
            QRegExp regx;
            if ("YYYY-MM-DD" == m_dateTextFormat.toUpper())
            {
                regx.setPattern("(([0-9]{3}[1-9]|[0-9]{2}[1-9][0-9]{1}|[0-9]{1}[1-9][0-9]"
                                "{2}|[1-9][0-9]{3})-(((0[13578]|1[02])-(0[1-9]|[12][0-9]|3"
                                "[01]))|((0[469]|11)-(0[1-9]|[12][0-9]|30))|(02-(0[1-9]|"
                                "[1][0-9]|2[0-8]))))|((([0-9]{2})(0[48]|[2468][048]|[13579]"
                                "[26])|((0[48]|[2468][048]|[3579][26])00))-02-29)");
            }
            else
            {
                regx.setPattern("(((0[1-9]|[12][0-9]|3[01])/((0[13578]|1[02]))|((0[1-9]|[12]"
                                "[0-9]|30)/(0[469]|11))|(0[1-9]|[1][0-9]|2[0-8])/(02))/([0-9]"
                                "{3}[1-9]|[0-9]{2}[1-9][0-9]{1}|[0-9]{1}[1-9][0-9]{2}|[1-9][0-9]"
                                "{3}))|(29/02/(([0-9]{2})(0[48]|[2468][048]|[13579][26])|((0[48]|"
                                "[2468][048]|[3579][26])00)))");
            }
            if (regx.exactMatch(text))
            {
                m_calendarWidget->setSelectedDate(QDate::fromString(text, m_dateTextFormat));
            }
        }

        void CDateEdit::recvReturnPressed()
        {
            qDebug()<<"CDateEdit::recvReturnPressed"<<endl;
        }

        void CDateEdit::recvSelectionChanged()
        {
            qDebug()<<"CDateEdit::recvSelectionChanged"<<endl;
            m_isSelect = true;
        }
