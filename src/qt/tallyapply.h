#ifndef TALLYAPPLY_H
#define TALLYAPPLY_H

#include <QWidget>
#include <QMessageBox>
#include "walletmodel.h"
class WalletModel;
namespace Ui {
class TallyApply;
}

class TallyApply : public QWidget
{
    Q_OBJECT

public:
    explicit TallyApply(QWidget *parent = 0);
    ~TallyApply();
    void setAddress(QString address);
    void setModel(WalletModel *_model);
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn);
    void resetinfo();
private Q_SLOTS:
    void on_pushButton_apply_pressed();

    void on_pushButton_address_pressed();

private:
    Ui::TallyApply *ui;
    WalletModel *model;

    bool m_isLocked;
    int eTag;
    std::string m_error;

Q_SIGNALS:
    void next(QString add,CAmount num);
    void selectaddress();
};

#endif // TALLYAPPLY_H
