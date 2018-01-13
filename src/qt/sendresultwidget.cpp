#include "sendresultwidget.h"
#include "ui_sendresultwidget.h"

SendResultWidget::SendResultWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SendResultWidget)
{
    ui->setupUi(this);
}

SendResultWidget::~SendResultWidget()
{
    delete ui;
}

void SendResultWidget::on_pushButton_pressed()
{
    Q_EMIT backmain();
}

