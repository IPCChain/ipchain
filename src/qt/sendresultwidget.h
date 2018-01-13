#ifndef SENDRESULTWIDGET_H
#define SENDRESULTWIDGET_H

#include <QWidget>

namespace Ui {
class SendResultWidget;
}

class SendResultWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SendResultWidget(QWidget *parent = 0);
    ~SendResultWidget();


Q_SIGNALS:
    void backmain();
private Q_SLOTS:
    void on_pushButton_pressed();

private:
    Ui::SendResultWidget *ui;

};

#endif // SENDRESULTWIDGET_H
