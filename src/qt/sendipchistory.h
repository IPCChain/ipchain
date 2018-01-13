#ifndef SENDIPCHISTORY_H
#define SENDIPCHISTORY_H

#include <QWidget>
QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE
namespace Ui {
class sendipchistory;
}

class sendipchistory : public QWidget
{
    Q_OBJECT

public:
    explicit sendipchistory(const QModelIndex &idx,QWidget *parent = 0);
    ~sendipchistory();
    void showVisual(bool visual);
    void updateInfo(QString status);

private:
    Ui::sendipchistory *ui;
};

#endif // SENDIPCHISTORY_H
