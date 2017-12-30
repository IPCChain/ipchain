#ifndef LOGON_H
#define LOGON_H

#include <QWidget>
#include <QMovie>
namespace Ui {
class logon;
}

class logon : public QWidget
{
    Q_OBJECT

public:
    explicit logon(QWidget *parent = NULL );
    ~logon();
    void showwaitpic();
    void timerEvent( QTimerEvent *event );
    int m_nTimerId;
    QMovie *m_movie;
    void setdatapath(QString);
    QString getdatapath();
    void stopLoadingTimer();
private Q_SLOTS:
    void on_pushButton_createwallet_pressed();

    void on_pushButton_restorefrombackup_pressed();

    void on_pushButton_changedata_pressed();

private:
    Ui::logon *ui;
Q_SIGNALS:
   void createwallet();
   void gotoRestorePage();
};

#endif // LOGON_H
