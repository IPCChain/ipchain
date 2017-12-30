#ifndef FEEWIDGET_H
#define FEEWIDGET_H

#include <QWidget>
#include <QModelIndex>
namespace Ui {
class FeeWidget;
}

class FeeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FeeWidget(QWidget *parent = 0);
    ~FeeWidget();
    Q_SIGNALS:
     void GotosendcoindAffirmPage(QString fee,QString fee1);

private Q_SLOTS:
    void on_tableWidget_activated(const QModelIndex &index);

    void on_tableWidget_doubleClicked(const QModelIndex &index);

    void on_pushButton_pressed();

    void on_pushButton_2_pressed();

private:
    Ui::FeeWidget *ui;
};

#endif // FEEWIDGET_H
