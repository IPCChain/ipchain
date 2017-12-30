#ifndef INFOWIDGET_H
#define INFOWIDGET_H

#include <QWidget>
#include <QDialog>
#include <QDateTime>
class ClientModel;
class WalletModel;
class OverviewPage;
namespace Ui {
class infowidget;
}

class InfoWidget : public QDialog//QWidget
{
    Q_OBJECT

public:

    explicit InfoWidget(QWidget *parent = 0);
    ~InfoWidget();

    static InfoWidget *GetInstance()
        {
            if( mm == NULL )
            {
                mm = new InfoWidget;
            }
            return mm;
        }

    void setWalletModel(WalletModel *walletModel){m_walletModel = walletModel;}
    void setClientModel(ClientModel *model);
    void setKnownBestHeight(int count, const QDateTime& blockDate);
    void setNumBlocks(int count, const QDateTime& blockDate, double nVerificationProgress, bool header);
    bool tipUpdate(int count, const QDateTime& blockDate, double nVerificationProgress);
    void showHide(bool hide = false);//, bool userRequested = false);
    void userRequestshowHide(bool userRequested = false);
    bool getuserRequest();
    void setuserRequest(bool userRequested);
    bool isLayerVisible() { return layerIsVisible; }
    void fresh(int count, const QDateTime& blockDate, double nVerificationProgress,bool head);
    QString getVersion();
    QString m_versionhtml;

Q_SIGNALS:
    void infoupdate(int count, const QDateTime& blockDate, double nVerificationProgress);
    void back();

    void linkActivated(QString url);

private Q_SLOTS:
   void  openUrll();
   void openUrl1();
   void openUrlVersion();


private:
    Ui::infowidget *ui;
    int bestHeaderHeight =0 ; //best known height (based on the headers)
    QDateTime bestHeaderDate;
    QVector<QPair<qint64, double> > blockProcessTime;
    bool layerIsVisible;
    bool userClosed;
    bool  m_userRequested;
    WalletModel *m_walletModel;
    ClientModel *clientModel;

    QString m_warnlabel_imagename;


    static InfoWidget* mm;
};

#endif // INFOWIDGET_H
