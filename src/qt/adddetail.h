#ifndef ADDDETAIL_H
#define ADDDETAIL_H
#include "walletmodel.h"
#include <QWidget>
#include <QPoint>

class WalletModel;

namespace Ui {
class AddDetail;
}

class AddDetail : public QWidget
{
    Q_OBJECT
public:
    explicit AddDetail(QWidget *parent = 0);
    ~AddDetail();
    void setModel(WalletModel *model);
Q_SIGNALS:
    void back();
protected:
    void showEvent(QShowEvent *event);
private Q_SLOTS:
    void on_backButton_pressed();

private:
    WalletModel *model;
    Ui::AddDetail *ui;
};

#endif // ADDDETAIL_H
