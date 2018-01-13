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
    void setMsg(QString s1);
Q_SIGNALS:
    void GoSendeCoinSuccess(QString s1);
private Q_SLOTS:
    void on_GoBtn_pressed();

private:
    Ui::eCoinSendresultDialog *ui;
    QString m_name;
};

#endif // ECOINSENDRESULTDIALOG_H
