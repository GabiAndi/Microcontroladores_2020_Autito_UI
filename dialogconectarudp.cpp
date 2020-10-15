#include "dialogconectarudp.h"
#include "ui_dialogconectarudp.h"

DialogConectarUDP::DialogConectarUDP(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogConectarUDP)
{
    ui->setupUi(this);
}

DialogConectarUDP::~DialogConectarUDP()
{
    delete ui;
}

void DialogConectarUDP::closeEvent(QCloseEvent *)
{
    deleteLater();
}

void DialogConectarUDP::setSystemManager(SystemManager *sys)
{
    this->sys = sys;
}

void DialogConectarUDP::setUdpSocket(QUdpSocket *udpSocket, QHostAddress *ip, quint16 *port)
{
    this->udpSocket = udpSocket;
    this->ip = ip;
    this->port = port;

    if (udpSocket->isOpen())
    {
        ui->lineEditIP->setText(ip->toString());
        ui->lineEditPuerto->setText(QString::asprintf("%d", *port));

        ui->lineEditIP->setEnabled(false);
        ui->lineEditPuerto->setEnabled(false);
        ui->pushButtonConectar->setText("Desconectar");
    }
}

void DialogConectarUDP::on_pushButtonCancelar_clicked()
{
    close();
}

void DialogConectarUDP::on_pushButtonConectar_clicked()
{
    if (udpSocket->isOpen())
    {
        udpSocket->close();

        ui->lineEditIP->setEnabled(true);
        ui->lineEditPuerto->setEnabled(true);
        ui->pushButtonConectar->setText("Conectar");

        sys->LOG("Socket UDP cerrado");
    }

    else
    {
        udpSocket->open(QIODevice::ReadWrite);
        udpSocket->abort();
        udpSocket->bind(ui->lineEditPuerto->text().toUInt());

        ip->setAddress(ui->lineEditIP->text());
        *port = ui->lineEditPuerto->text().toUInt();

        ui->lineEditIP->setEnabled(false);
        ui->lineEditPuerto->setEnabled(false);
        ui->pushButtonConectar->setText("Desconectar");

        sys->LOG("Socket UDP abierto:\r\n\tIP: " + ip->toString() + "\r\n\tPuerto: " + QString::number(*port));
    }
}
