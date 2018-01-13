#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QWidget>

namespace Ui {
class exportdialog;
}
class WalletModel;
class exportdialog : public QWidget
{
    Q_OBJECT

public:
    explicit exportdialog(QWidget *parent = 0);
    ~exportdialog();
    WalletModel *walletModel;
    void setWalletModel(WalletModel * model){walletModel = model;}
    void clearInfo();
    bool copyFileToPath(QString sourceDir ,QString toDir, bool coverFileIfExist);
    QString setDestSourcePath(QString path);
public Q_SLOTS:
    void on_pushButton_back_pressed();
Q_SIGNALS:
    void back();
    void confirm(int);
private Q_SLOTS:
    void on_pushButton_confirm_pressed();
    void on_pushButton_other_pressed();
    void on_pushButton_browse_pressed();

private:
    Ui::exportdialog *ui;
    bool showother;
    void showorhideother();
    QString m_file_full;
};

#endif // EXPORTDIALOG_H
