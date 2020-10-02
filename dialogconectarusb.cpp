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
            QMessageBox(QMessageBox::Icon::Information, "Conexión establecida", "Se conecto a " + serialPort->portName(),
                        QMessageBox::Button::Ok, this).exec();

            ui->comboBoxPuertos->setEnabled(false);
            ui->pushButtonActualizarPuertos->setEnabled(false);
            ui->pushButtonConectar->setText("Desconectar");
        }

        else
        {
            QMessageBox(QMessageBox::Icon::Critical, "Conexión erronea", "No se conecto a " + serialPort->portName(),
                        QMessageBox::Button::Ok, this).exec();
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
