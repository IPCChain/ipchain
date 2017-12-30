#ifndef TALLYCLAUSE_H
#define TALLYCLAUSE_H

#include <QWidget>

namespace Ui {
class TallyClause;
}

class TallyClause : public QWidget
{
    Q_OBJECT

public:
    explicit TallyClause(QWidget *parent = 0);
    ~TallyClause();

private Q_SLOTS:
    void on_pushButton_OK_pressed();

    void on_pushButton_Cancle_pressed();

    void on_checkBox_hacereaded_stateChanged(int arg1);

private:
    Ui::TallyClause *ui;
Q_SIGNALS:
    void next();
    void back();
};

#endif // TALLYCLAUSE_H
