#include "dialogconectarusb.h"
#include "ui_dialogconectarusb.h"

DialogConectarUSB::DialogConectarUSB(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogConectarUSB)
{
    ui->setupUi(this);
}

DialogConectarUSB::~DialogConectarUSB()
{
    delete ui;
}

void DialogConectarUSB::closeEvent(QCloseEvent *)
{
    deleteLater();
}

void DialogConectarUSB::setSerialPort(QSerialPort *serialPort)
{
    this->serialPort = serialPort;

    if (serialPort->isOpen())
    {
        ui->comboBoxPuertos->addItem(this->serialPort->portName());
        ui->comboBoxPuertos->setEnabled(false);
        ui->pushButtonActualizarPuertos->setEnabled(false);
        ui->pushButtonConectar->setText("Desconectar");
    }
}

void DialogConectarUSB::on_pushButtonActualizarPuertos_clicked()
{
    ui->comboBoxPuertos->clear();

    for (QSerialPortInfo port : QSerialPortInfo::availablePorts())
    {
        ui->comboBoxPuertos->addItem(port.portName());
    }
}

void DialogConectarUSB::on_pushButtonCancelar_clicked()
{
    close();
}

void DialogConectarUSB::on_pushButtonConectar_clicked()
{
    if (!serialPort->isOpen())
    {
        serialPort->setPortName(ui->comboBoxPuertos->currentText());
        serialPort->setBaudRate(115200);
        serialPort->setParity(QSerialPort::Parity::NoParity);
        serialPort->setStopBits(QSerialPort::StopBits::OneStop);

        if (serialPort->open(QIODevice::ReadWrite))
        {
            QMessageBox *messageBox = new QMessageBox(this);

            messageBox->setAttribute(Qt::WA_DeleteOnClose);
            messageBox->setIcon(QMessageBox::Icon::Information);
            messageBox->setWindowTitle("Conexión establecida");
            messageBox->setText("Se conecto a " + serialPort->portName());
            messageBox->setStandardButtons(QMessageBox::Button::Ok);

            messageBox->open();

            ui->comboBoxPuertos->setEnabled(false);
            ui->pushButtonActualizarPuertos->setEnabled(false);
            ui->pushButtonConectar->setText("Desconectar");
        }

        else
        {
            QMessageBox *messageBox = new QMessageBox(this);

            messageBox->setAttribute(Qt::WA_DeleteOnClose);
            messageBox->setIcon(QMessageBox::Icon::Information);
            messageBox->setWindowTitle("Conexión erronea");
            messageBox->setText("No se conecto a " + serialPort->portName());
            messageBox->setStandardButtons(QMessageBox::Button::Ok);

            messageBox->open();
        }
    }

    else
    {
        serialPort->close();

        ui->comboBoxPuertos->setEnabled(true);
        ui->pushButtonActualizarPuertos->setEnabled(true);
        ui->pushButtonConectar->setText("Conectar");
    }
}
