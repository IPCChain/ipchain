#ifndef SUCCESSFULTRADE_H
#define SUCCESSFULTRADE_H

#include <QWidget>

namespace Ui {
class SuccessfulTrade;
}

class SuccessfulTrade : public QWidget
{
    Q_OBJECT

public:
    explicit SuccessfulTrade(QWidget *parent = 0);
    ~SuccessfulTrade();
    enum SuccessText{
        successtrade,
        successexport,
        successrevovery,
        successpsdset,
        successimport
    };
    void setSuccessText(int);
    void setsendSuccessText_(QString s1,QString s2);
    void setSuccessText_(QString s1,QString s2);
private Q_SLOTS:
    void on_pushButton_ok_pressed();

    void on_btn_import_pressed();

Q_SIGNALS:
    void back();
private:
    Ui::SuccessfulTrade *ui;
};

#endif // SUCCESSFULTRADE_H
