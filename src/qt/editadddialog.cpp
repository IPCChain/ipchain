#include "editadddialog.h"
#include "ui_editadddialog.h"
#include "addresstablemodel.h"
#include <QDataWidgetMapper>
#include <QMessageBox>
editadddialog::editadddialog(QWidget *parent) :
    //  QWidget(parent),
    QDialog(parent),
    ui(new Ui::editadddialog),
    mapper(0)
{

    ui->setupUi(this);
    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    mode = NewSendingAddress;
    inputDataList=new  QList<QString>();

    ui->nameEdit->setPlaceholderText("Input name");
    ui->AddressEdit->setPlaceholderText("Input address");

}

editadddialog::~editadddialog()
{
    delete ui;
}
void editadddialog::settag(int tag)
{
    etag= tag;
}
int editadddialog::gettag()
{
    return etag;
}
void editadddialog::setModel(AddressTableModel *_model)
{

    this->model = _model;
    if(!_model)
        return;
    mapper->setModel(_model);
    mapper->addMapping(ui->nameEdit, AddressTableModel::Label);
    mapper->addMapping(ui->AddressEdit, AddressTableModel::Address);

}
void editadddialog::on_AddButton_pressed()
{
    inputDataList->append(ui->nameEdit->text());
    inputDataList->append(ui->AddressEdit->text());

    accept();

}


void editadddialog::loadRow(int row)
{
    mapper->setCurrentIndex(row);
}

bool editadddialog::saveCurrentRow()
{
    if(!model)
        return false;

    address = model->addRow( AddressTableModel::Send, ui->nameEdit->text(),ui->AddressEdit->text());
    return !address.isEmpty();
}

void editadddialog::accept()
{

    int a = gettag();
    if(!model)
        return;
    if(!saveCurrentRow())
    {
        switch(model->getEditStatus())
        {
        case AddressTableModel::OK:
            Q_EMIT sendDataList(inputDataList,a);//20170821
            break;
        case AddressTableModel::NO_CHANGES:
            // No changes were made during edit operation. Just reject.
            break;
        case AddressTableModel::INVALID_ADDRESS:
            QMessageBox::warning(this, windowTitle(),
                                 tr("The entered address \"%1\" is not a valid Bitcoin address.").arg(ui->AddressEdit->text()),
                                 QMessageBox::Ok, QMessageBox::Ok);
            Q_EMIT sendDataList(inputDataList,a);
            break;
        case AddressTableModel::DUPLICATE_ADDRESS:
            QMessageBox::warning(this, windowTitle(),
                                 tr("The entered address \"%1\" is already in the address book.").arg(ui->AddressEdit->text()),
                                 QMessageBox::Ok, QMessageBox::Ok);
            break;
        case AddressTableModel::WALLET_UNLOCK_FAILURE:
            QMessageBox::critical(this, windowTitle(),
                                  tr("Could not unlock wallet."),
                                  QMessageBox::Ok, QMessageBox::Ok);
            break;
        case AddressTableModel::KEY_GENERATION_FAILURE:
            QMessageBox::critical(this, windowTitle(),
                                  tr("New key generation failed."),
                                  QMessageBox::Ok, QMessageBox::Ok);
            break;

        }
        return;
    }
    Q_EMIT sendDataList(inputDataList,a);//20170821
    QDialog::accept();

}

QString editadddialog::getAddress() const
{
    return address;
}

void editadddialog::setAddress(const QString &_address)
{
    this->address = _address;
    ui->AddressEdit->setText(_address);
}


