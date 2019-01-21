#ifndef SETIMPORT_H
#define SETIMPORT_H

#include <QWidget>
class WalletModel;
namespace Ui {
class setimport;
}

class setimport : public QWidget
{
    Q_OBJECT

public:
    explicit setimport(QWidget *parent = 0);
    ~setimport();
    void setWalletModel(WalletModel * model);
Q_SIGNALS:
    void back();
    void confirm(int);
public Q_SLOTS:
    void on_pushButton_import_pressed();

private:
    Ui::setimport *ui;
    WalletModel *walletModel;
};

#endif // SETIMPORT_H
