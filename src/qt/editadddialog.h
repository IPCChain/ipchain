#ifndef EDITADDDIALOG_H
#define EDITADDDIALOG_H

class AddressTableModel;


//#include <QWidget>
#include <QDialog>
QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
QT_END_NAMESPACE
namespace Ui {
class editadddialog;
}

class editadddialog : public QDialog//public QWidget
{
    Q_OBJECT

public:


    enum Mode {
        NewReceivingAddress,
        NewSendingAddress,
        EditReceivingAddress,
        EditSendingAddress
    };
Mode mode;
    explicit editadddialog(QWidget *parent = 0);
    ~editadddialog();

    void setModel(AddressTableModel *model);
    void loadRow(int row);

    QString getAddress() const;
    void setAddress(const QString &address);
bool saveCurrentRow();

void settag(int tag);

int gettag();

Q_SIGNALS:
    void sendCoins(QString addr);
     void sendDataList(QList<QString> *inputDataList,int tag);
private Q_SLOTS:
    void on_AddButton_pressed();
 void accept();
private:
 int etag;
    Ui::editadddialog *ui;
     AddressTableModel *model;
      QDataWidgetMapper *mapper;
         QString address;
         QList<QString> *inputDataList;
};

#endif // EDITADDDIALOG_H
