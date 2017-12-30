#ifndef LOGONDLG_H
#define LOGONDLG_H

#include <QDialog>
class SetRecovery;
class logon;
namespace Ui {
class logondlg;
}

class logondlg : public QDialog
{
    Q_OBJECT

public:
    explicit logondlg(QWidget *parent = 0);
    ~logondlg();
    logon *m_plogon;
    SetRecovery *m_pSetRecovery;
    void setDlgParams();
    void showwindowwithwait();
    void setdatapath(QString);
    QString getdatapath();

private:
    Ui::logondlg *ui;


public Q_SLOTS:
    void createwallet();
    void slotFinish(QWidget*);
    void backcreatewallet();
    void gotoRestorePage();
};

#endif // LOGONDLG_H
