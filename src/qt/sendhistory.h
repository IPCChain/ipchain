#ifndef SENDHISTORY_H
#define SENDHISTORY_H

#include <QWidget>
#include <QDialog>
#include <QObject>
QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE
namespace Ui {
class sendhistory;
}

class sendhistory : public QDialog
{
    Q_OBJECT

public:
     explicit sendhistory(const QModelIndex &idx, QWidget *parent = 0);
    ~sendhistory();

    void showVisual(bool visual);

    void updateInfo(QString status);

private:
    Ui::sendhistory *ui;
};

#endif // SENDHISTORY_H
