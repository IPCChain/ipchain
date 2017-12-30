#ifndef CMESSAGEBOX_H
#define CMESSAGEBOX_H

#include <QDialog>

namespace Ui {
class CMessageBox;
}

class CMessageBox : public QDialog
{
    Q_OBJECT

public:
    explicit CMessageBox(QWidget *parent = 0);
    ~CMessageBox();
    void setMessage(QString msg);
    void setMessage(int msg);
    int m_answertype;

private Q_SLOTS:
    void on_pushButton_ok_pressed();

    void on_pushButton__cancle_pressed();

private:
    Ui::CMessageBox *ui;
};

#endif // CMESSAGEBOX_H
