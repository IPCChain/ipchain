#ifndef UPGRADEWIDGET_H
#define UPGRADEWIDGET_H
#include "amount.h"
#include <QObject>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
class upgradewidget : public QWidget
{
    Q_OBJECT
public:
    explicit upgradewidget(QWidget *parent = 0);
    void focusInEvent(QFocusEvent *e);
    void focusOutEvent(QFocusEvent *e);

    enum TabTypes {
        TAB_one = 1,
        TAB_two = 2,
        TAB_three = 3,
        TAB_four = 4,
        TAB_five = 5

    };

    struct eCoinMsg{
        int index;
        std::string txId;
        QString name;
        QString add;
        int num;

    };

    void setadd(QString add);
    void setlabel(QString label);
    void setIntertag(int tag);
    int getIntertag();
    void setMessage(eCoinMsg  eMsg);

    int m_dialognum;
    QLabel* getipcselectlabel();
    void setipcselectlabel(QLabel*data){pipcselectlabel = data;}
    void setunioninfo(std::string add, CAmount fee,bool isSend,CAmount num,QString status,QString strtime);

    QString m_txid;
    bool m_isunion = false;

    QString name,add,m_name;

Q_SIGNALS:
  void pressed();
  void pressw(QString add,QString label);

  void selectinfo(QString add,QString label);

  void pressecoin(upgradewidget::eCoinMsg eMsg);
  void dlgnum(int);
  void dlgtxid(QString);
  void selectlabel(QLabel*);
  void updateinfo(QString tag,QString add);

  void lookinfo(std::string add, bool isSend,QString status,QString strtime,
                CAmount s1,CAmount s2,QString txid);
public Q_SLOTS:
  void myPressedDown();

  void changetext(QString tx);
protected:
    void mousePressEvent(QMouseEvent *e);

private:
    QLabel *pipcselectlabel;
    QString m_add;
    QString m_label;
    int eInterTag;
    eCoinMsg  eMsg_;




    std::string m_unionadd; CAmount  m_unionfee;
    bool  m_unionisSend;CAmount  m_unionnum;QString  m_unionstatus;QString  m_unionstrtime;

};

#endif // UPGRADEWIDGET_H
