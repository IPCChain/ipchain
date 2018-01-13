#include "settingwidget.h"
#include "ui_settingwidget.h"

Settingwidget::Settingwidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Settingwidget)
{
    ui->setupUi(this);
}

Settingwidget::~Settingwidget()
{
    delete ui;
}

void Settingwidget::on_GoSetButton_pressed()
{
    Q_EMIT openPasswordSetwidget(2);
}

void Settingwidget::on_NoSetButton_pressed()
{
    Q_EMIT openSendCoinsAffrimwidget();
}
