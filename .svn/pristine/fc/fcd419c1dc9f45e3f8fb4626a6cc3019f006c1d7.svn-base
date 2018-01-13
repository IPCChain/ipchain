#include "tallyclause.h"
#include "forms/ui_tallyclause.h"
#include <QLocale>
extern const char* g_clause ;
extern const char* g_clause_en ;
TallyClause::TallyClause(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TallyClause)
{
    ui->setupUi(this);
    ui->pushButton_OK->setEnabled(0);
    ui->pushButton_Cancle->setEnabled(0);
    ui->textEdit->setReadOnly(true);

    QLocale locale;
    if( locale.language() == QLocale::Chinese )
    {
#ifdef WIN32
        ui->textEdit->setText(QString(g_clause));
#else
        ui->textEdit->setText(QString::fromLocal8Bit(g_clause));
#endif

    }
    else
    {
#ifdef WIN32
        ui->textEdit->setText(QString(g_clause_en));
#else
        ui->textEdit->setText(QString::fromLocal8Bit(g_clause_en));
#endif
    }
}

TallyClause::~TallyClause()
{
    delete ui;
}

void TallyClause::on_pushButton_OK_pressed()
{
    Q_EMIT next();
}

void TallyClause::on_pushButton_Cancle_pressed()
{
    Q_EMIT back();
}

void TallyClause::on_checkBox_hacereaded_stateChanged(int arg1)
{
    if(arg1){
        ui->pushButton_OK->setEnabled(1);
        ui->pushButton_Cancle->setEnabled(1);
    }else{
        ui->pushButton_OK->setEnabled(0);
        ui->pushButton_Cancle->setEnabled(0);
    }
}
