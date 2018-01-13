#include "tallydscribe.h"
#include "ui_tallydscribe.h"
#include "contract.h"
#include "log/log.h"
#include <QLocale>
TallyDscribe::TallyDscribe(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TallyDscribe)
{
    ui->setupUi(this);

    QLocale locale;
    if( locale.language() == QLocale::Chinese )
    {
        #ifdef WIN32
            ui->textEdit->setText(QString(g_contract));
        #else
            ui->textEdit->setText(QString::fromLocal8Bit(g_contract));
        #endif
    }
    else
    {
        #ifdef WIN32
            ui->textEdit->setText(QString(g_contract_en));
        #else
            ui->textEdit->setText(QString::fromLocal8Bit(g_contract_en));
        #endif
    }
    ui->textEdit->setReadOnly(true);
}

TallyDscribe::~TallyDscribe()
{
    delete ui;
}

void TallyDscribe::on_pushButton_wantaccount_pressed()
{
    Q_EMIT next();
}
