#ifndef SETMESSAGESIGNATURE_H
#define SETMESSAGESIGNATURE_H

#include <QWidget>

namespace Ui {
class SetMessageSignature;
}
class WalletModel;
class SetMessageSignature : public QWidget
{
    Q_OBJECT

public:
    explicit SetMessageSignature(QWidget *parent = 0);
    ~SetMessageSignature();
    WalletModel *walletModel;
    void setModel(WalletModel * model){walletModel = model;}
private Q_SLOTS:
    void on_pushButton_sign_pressed();

private:
    Ui::SetMessageSignature *ui;
};

#endif // SETMESSAGESIGNATURE_H
