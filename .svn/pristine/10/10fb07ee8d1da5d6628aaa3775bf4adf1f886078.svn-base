#include "unionaccounttrasend.h"
#include "ui_unionaccounttrasend.h"
#include "log/log.h"
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include "intro.h"
#ifdef WIN32
#include <windows.h>
const std::string DIRECTORY_SEPARATOR = "\\";
#else
const std::string DIRECTORY_SEPARATOR = "/";
#endif

#ifdef WIN32
static void CreateDir(const std::string& path)
{
    CreateDirectoryA(path.c_str(), NULL);
}
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PATH 512


static void CreateDir(const std::string& path)
{
    mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

#endif
unionaccounttrasend::unionaccounttrasend(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::unionaccounttrasend)
{
    ui->setupUi(this);

    QRegExp regx("^\\d+(\\.\\d+)?$");
    QValidator *validator = new QRegExpValidator(regx, ui->lineEdit_num );
    ui->lineEdit_num->setValidator( validator );
}

unionaccounttrasend::~unionaccounttrasend()
{
    delete ui;
}
void unionaccounttrasend::setModel(WalletModel *_model)
{
    this->model = _model;
}
void unionaccounttrasend::setaddress(QString add)
{
    ui->lineEdit_add->setText("");
    ui->lineEdit_num->setText("");
    ui->textEdit->setText("");
    ui->label_tip->setText("");
    m_add =add;
}
QString unionaccounttrasend::getaddress()
{
    return m_add;
}
void unionaccounttrasend::on_sendbtn_pressed()
{


    ui->label_tip->setText("");
    m_sendcoinerror = "";
    if(!model || !model->getOptionsModel())
        return;
    if("" == ui->lineEdit_num->text() ||
            "" == ui->lineEdit_add->text())
    {
        ui->label_tip->setText(tr("input info"));
        return;
    }

    if(!model->CheckPassword())
    {
        ui->label_tip->setText(tr("Password error."));
        LOG_WRITE(LOG_INFO,"on_sendbtn_pressed Password error.");
        return;
    }
    WalletModel::UnlockContext ctx(model, true, true);

    // CAmount t = (ui->lineEdit_num->text()).toDouble();//num
    QString t = ui->lineEdit_num->text();
    int h= t.indexOf(" ");
    QString s = t.mid(0,h);
    double num =  s.toDouble();
    num=num*100000000;
    QString add = ui->lineEdit_add->text();
    ScriptError serror;
    std::string strtx = "";
    QString add_from = getaddress();
   // LOG_WRITE(LOG_INFO,"SendMul num*00000",QString::number(num).toStdString().c_str());

    WalletModel::SendCoinsReturn sendStatus =model->repreparUnionaccountCreate(add_from,add,num,serror,strtx);

    LOG_WRITE(LOG_INFO,"sendStatus",QString::number(sendStatus.status).toStdString().c_str());
    if( WalletModel::OK == sendStatus.status )
    {
        if(ScriptError::SCRIPT_ERR_OK == serror)
        {
            std::string strFailReason;
            ScriptError serror;
            std::string strFailReason_plus;
            std::string strtxout;
            CMutableTransaction mergedTx;
            if(model->commitUnionaccounttra( strtx, strFailReason,serror,strtxout,mergedTx))
            {
                if(ScriptError::SCRIPT_ERR_OK == serror)
                {
                    if(model->commitUnionaccounttra_plus(mergedTx,strFailReason_plus))
                    {
                        Q_EMIT opensendsuccessPage(tr("sign successed"),"");
                    }
                }
                else
                {
                    std::string path = Intro::m_datapath;
                    if(path == "")return;
                    CreateDir(path);

                    std::string logFile = path +DIRECTORY_SEPARATOR+ "address.txt";

                    FILE* m_file = fopen(logFile.c_str(), "a+");


                    fprintf(m_file, "address: %s\n", add_from.toStdString().c_str());
                    fprintf(m_file, "script: %s\n",QString::fromStdString(strtx).toStdString().c_str() );

                    fclose(m_file);


                    Q_EMIT opensendsuccessPage(tr("create successed"), QString::fromStdString(strtxout));
                }
            }
            else
            {
                LOG_WRITE(LOG_INFO,"SEND sign error");
                ui->label_tip->setText(tr("sign error"));
            }
        }
        else
        {

            std::string path = Intro::m_datapath;
            if(path == "")return;
            CreateDir(path);

            std::string logFile = path +DIRECTORY_SEPARATOR+ "address.txt";
/*
            QFile file(logFile.c_str());
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                LOG_WRITE(LOG_INFO,"open fail");
                return;
            }
            else
            {
                QString str1, str2, qstr_similarity, sumqstr;
                for(int i = 0; i < 1; ++i)
                {
                    str1 = QString::fromStdString(ui->textEdit->document()->toPlainText().toStdString());

                    sumqstr = "address:"+add_from+"\n"+"script:" + QString::fromStdString(strtx) + "\n" ;
                }
                QTextStream in(&file);
                in << sumqstr;
                file.close();

            }
            */
            FILE* m_file = fopen(logFile.c_str(), "a+");


            fprintf(m_file, "address: %s\n", add_from.toStdString().c_str());
            fprintf(m_file, "script: %s\n",QString::fromStdString(strtx).toStdString().c_str() );
            fclose(m_file);


            Q_EMIT opensendsuccessPage(tr("create successed"), QString::fromStdString(strtx));

        }

    }
    else
    {

        m_sendcoinerror =  model->m_sendcoinerror;
        LOG_WRITE(LOG_INFO,"error",m_sendcoinerror.c_str());
        processSendCoinsReturn(sendStatus);
    }

}
void unionaccounttrasend::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn)
{
    QPalette pa;
    pa.setColor(QPalette::WindowText,Qt::black);

    switch(sendCoinsReturn.status)
    {
    case WalletModel::PsdErr:
        ui->label_tip->setText(tr("password error"));
        break;
    case WalletModel::InvalidAddress:
        ui->label_tip->setText(tr("The recipient address is not valid. Please recheck."));
        break;
    case WalletModel::InvalidAmount:
        ui->label_tip->setText(tr("Invalid Amount!"));
        break;
    case WalletModel::AmountExceedsBalance:
        ui->label_tip->setText(tr("The amount exceeds your balance."));
        break;
    case WalletModel::AmountWithFeeExceedsBalance:
        // ui->label_tip->setText(tr("The total exceeds your balance when the 0.001IPC/KB transaction fee is included.").arg(msgArg));
        break;
    case WalletModel::DuplicateAddress:
        ui->label_tip->setText(tr("Duplicate address found: addresses should only be used once each."));
        break;
    case WalletModel::TransactionCreationFailed:
        if(m_sendcoinerror!="")
            ui->label_tip->setText(m_sendcoinerror.c_str());
        else
            ui->label_tip->setText(tr("Transaction creation failed!"));
        break;
    case WalletModel::TransactionCommitFailed:
    {
        if("bad-Token-tokensymbol-repeat" == sendCoinsReturn.reasonCommitFailed )
        {
            ui->label_tip->setText(tr("The transaction was rejected with the following reason: bad-Token-tokensymbol-repeat"));

        }
        else if("bad-Token-tokenhash-repeat" == sendCoinsReturn.reasonCommitFailed )
        {
            ui->label_tip->setText(tr("The transaction was rejected with the following reason: bad-Token-tokenhash-repeat"));

        }
        else if("bad-Token-Multi-inType" == sendCoinsReturn.reasonCommitFailed )
        {
            ui->label_tip->setText(tr("The transaction was rejected with the following reason: bad-Token-Multi-inType"));

        }
        else if("bad-Token-Reg-issueDate(Regtime)" == sendCoinsReturn.reasonCommitFailed)
        {
            ui->label_tip->setText(tr("The transaction was rejected with the following reason: bad-Token-Reg-issueDate(Regtime)"));

        }
        else if("bad-Token-regtotoken-value-unequal" == sendCoinsReturn.reasonCommitFailed )
        {
            ui->label_tip->setText(tr("The transaction was rejected with the following reason: bad-Token-regtotoken-value-unequal"));
        }
        else if("bad-Token-Label-contain-errvalue"== sendCoinsReturn.reasonCommitFailed)
        {
            ui->label_tip->setText(tr("The transaction was rejected with the following reason: bad-Token-Label-contain-errvalue"));
        }
        else if("bad-Token-Symbol-contain-errvalue"== sendCoinsReturn.reasonCommitFailed)
        {
            ui->label_tip->setText(tr("The transaction was rejected with the following reason: bad-Token-Symbol-contain-errvalue"));
        }
        else if("bad-Token-value-unequal" == sendCoinsReturn.reasonCommitFailed )
        {
            ui->label_tip->setText(tr("The transaction was rejected with the following reason: bad-Token-value-unequal"));
        }
        else if("bad-Token-Multi-outType" == sendCoinsReturn.reasonCommitFailed )
        {
            ui->label_tip->setText(tr("The transaction was rejected with the following reason: bad-Token-Multi-outType"));
        }
        else
        {
            ui->label_tip->setText(tr("The transaction was rejected with the following reason: %1").arg(sendCoinsReturn.reasonCommitFailed));
        }
    }
        break;
    case WalletModel::AbsurdFee:
        ui->label_tip->setText(tr("A fee higher is considered an absurdly high fee."));
        break;
    case WalletModel::PaymentRequestExpired:
        ui->label_tip->setText(tr("Payment request expired."));
        break;
    case WalletModel::OK:
    default:
        return;
    }
}

void unionaccounttrasend::on_pushButton_back_pressed()
{
    Q_EMIT backtotraPage();
}
