#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include <QDialog>

namespace Ui {
class UpdateDialog;
}

class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpdateDialog(QWidget *parent = 0);
    ~UpdateDialog();
    void setinfo(QString type,QString sugtext,QString url);

private Q_SLOTS:
    void on_pushButton_goUpdate_pressed();

    void on_pushButton_laterUpdate_pressed();

private:
    Ui::UpdateDialog *ui;
};

#endif // UPDATEDIALOG_H
