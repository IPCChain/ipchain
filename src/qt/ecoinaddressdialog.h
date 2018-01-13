#ifndef ECOINADDRESSDIALOG_H
#define ECOINADDRESSDIALOG_H

#include <QWidget>

namespace Ui {
class EcoinAddressDialog;
}

class EcoinAddressDialog : public QWidget
{
    Q_OBJECT

public:
    explicit EcoinAddressDialog(QWidget *parent = 0);
    ~EcoinAddressDialog();
    void setAddress(QString);
private:
    Ui::EcoinAddressDialog *ui;
public Q_SLOTS:
    void pushButtonPressedBack();
Q_SIGNALS:
    void back();
};

#endif // ECOINADDRESSDIALOG_H
