#ifndef ECOINSENDRESULTDIALOG_H
#define ECOINSENDRESULTDIALOG_H

#include <QWidget>

namespace Ui {
class eCoinSendresultDialog;
}

class eCoinSendresultDialog : public QWidget
{
    Q_OBJECT

public:
    explicit eCoinSendresultDialog(QWidget *parent = 0);
    ~eCoinSendresultDialog();
Q_SIGNALS:
    void GoSendeCoinSuccess();
private Q_SLOTS:
    void on_GoBtn_pressed();

private:
    Ui::eCoinSendresultDialog *ui;
};

#endif // ECOINSENDRESULTDIALOG_H
