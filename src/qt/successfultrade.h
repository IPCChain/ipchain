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
        successpsdset
    };
    void setSuccessText(int);


private Q_SLOTS:
    void on_pushButton_ok_pressed();
Q_SIGNALS:
    void back();
private:
    Ui::SuccessfulTrade *ui;
};

#endif // SUCCESSFULTRADE_H
