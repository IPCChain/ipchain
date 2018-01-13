#ifndef SETRECOVERY_H
#define SETRECOVERY_H

#include <QWidget>
class WalletModel;
namespace Ui {
class SetRecovery;
}

class SetRecovery : public QWidget
{
    Q_OBJECT

public:
    explicit SetRecovery(QWidget *parent = 0);
    ~SetRecovery();
    WalletModel *walletModel;
    void setWalletModel(WalletModel * model);
     void setDestSourcePath(QString path);
    static QString m_setRecoveryPath;
private Q_SLOTS:
    void on_pushButton_import_pressed();
    void on_pushButton_select_pressed();
    void on_pushButton_back_pressed();
Q_SIGNALS:
    void back();
    void next();
    void success(int);

private:
    Ui::SetRecovery *ui;
    QString m_file_full;
    bool setCreatePage;
    QString m_desPath;

    bool isDirExist(QString fullPath);
    bool copyFileToPath(QString sourceDir ,QString toDir, bool coverFileIfExist);
};

#endif // SETRECOVERY_H
