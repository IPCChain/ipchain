#ifndef TALLYDSCRIBE_H
#define TALLYDSCRIBE_H

#include <QWidget>

namespace Ui {
class TallyDscribe;
}

class TallyDscribe : public QWidget
{
    Q_OBJECT

public:
    explicit TallyDscribe(QWidget *parent = 0);
    ~TallyDscribe();

private Q_SLOTS:
    void on_pushButton_wantaccount_pressed();

private:
    Ui::TallyDscribe *ui;

Q_SIGNALS:
    void next();
};

#endif // TALLYDSCRIBE_H
