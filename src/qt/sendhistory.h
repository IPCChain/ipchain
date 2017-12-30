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

class sendhistory : public QDialog//QWidget
{
    Q_OBJECT

public:
  //  explicit sendhistory(QWidget *parent = 0);
     explicit sendhistory(const QModelIndex &idx, QWidget *parent = 0);
    ~sendhistory();

    void showVisual(bool visual);
Q_SIGNALS:
    void backMain();
private Q_SLOTS:
    void on_pushButton_pressed();

private:
    Ui::sendhistory *ui;
};

#endif // SENDHISTORY_H
