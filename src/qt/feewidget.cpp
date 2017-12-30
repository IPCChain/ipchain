#include "feewidget.h"
#include "ui_feewidget.h"
#include <QMessageBox>
FeeWidget::FeeWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FeeWidget)
{
    ui->setupUi(this);
}

FeeWidget::~FeeWidget()
{
    delete ui;
}

void FeeWidget::on_tableWidget_activated(const QModelIndex &index)
{

}

void FeeWidget::on_tableWidget_doubleClicked(const QModelIndex &index)
{


}

void FeeWidget::on_pushButton_pressed()
{
    QString str1 = "ecmi";
    QString str2 = "2.6";

    Q_EMIT GotosendcoindAffirmPage(str2,str1);

}

void FeeWidget::on_pushButton_2_pressed()
{

    QString str1 = "ecme";
    QString str2 = "3.0";

    Q_EMIT GotosendcoindAffirmPage(str2,str1);

}
