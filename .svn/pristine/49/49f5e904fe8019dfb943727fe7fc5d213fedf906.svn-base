#include "unionaccountgenkey.h"
#include "ui_unionaccountgenkey.h"
#include "log/log.h"
#ifdef ENABLE_WALLET
#include "walletmodel.h"
#endif
unionaccountgenkey::unionaccountgenkey(QWidget *parent) :
    QWidget(parent),model(NULL),
    ui(new Ui::unionaccountgenkey)
{
    ui->setupUi(this);
    ui->genkey->setVisible(false);
    ui->key_label->setText("");
}

unionaccountgenkey::~unionaccountgenkey()
{
    delete ui;
}
void unionaccountgenkey::setModel(WalletModel *_model)
{
    this->model = _model;
}
void unionaccountgenkey::on_genkey_pressed()
{

}

void unionaccountgenkey::on_selectadd_btn_pressed()
{
    Q_EMIT selectaddress();
}
void unionaccountgenkey::setaddress(QString address)
{
    m_address = address;
    ui->selectadd_btn->setText(address);
    ui->key_label->setText("");


    std::string m_key;
    std::string m_failreason;
    std::string add = m_address.toStdString();
    if("" == add)
    {
        ui->label_err->setText("input add");
        return;
    }
    if(this->model->addtopubkey(add,m_key,m_failreason))
    {
        LOG_WRITE(LOG_INFO,"GENKEY",m_key.c_str());
        ui->key_label->setText(QString::fromStdString(m_key));
    }
    else
    {

        if("address is valid!" ==m_failreason )
        {
            ui->label_err->setText(tr("address is valid!"));
        }
        else if("address can't be Script!" ==m_failreason)
        {
            ui->label_err->setText(tr("address can't be Script!"));
        }
        else if("GetPubKey faild!" ==m_failreason)
        {
            ui->label_err->setText(tr("GetPubKey faild!"));
        }
        else
        {
            ui->label_err->setText(tr("GenKey faild!"));
        }
    }

}

void unionaccountgenkey::clearData()
{
    m_address = "";
}

void unionaccountgenkey::showEvent(QShowEvent *event)
{
    ui->label_err->setText("");

}
void unionaccountgenkey::clearinfo(){
    ui->label_err->setText("");
    ui->key_label->setText("");
    ui->selectadd_btn->setText(tr("select address"));
}
