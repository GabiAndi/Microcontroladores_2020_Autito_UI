#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    serialPort = new QSerialPort();
    udpSocket = new QUdpSocket();
}

MainWindow::~MainWindow()
{
    delete serialPort;
    delete udpSocket;

    delete ui;
}

void MainWindow::on_actionUSB_triggered()
{
    if (serialPort->isOpen())
    {
        mainWindowDepuracionUSB = new MainWindowDepuracionUSB(this);
        mainWindowDepuracionUSB->setSerialPort(serialPort);
        mainWindowDepuracionUSB->show();
    }

    else
    {
        QMessageBox(QMessageBox::Icon::Warning, "Error al iniciar la depuración", "Primero conectese a un puerto serie",
                    QMessageBox::Button::Ok, this).exec();

        dialogConectarUSB = new DialogConectarUSB(this);
        dialogConectarUSB->setSerialPort(serialPort);
        dialogConectarUSB->show();
    }
}

void MainWindow::on_actionUDP_triggered()
{
    if (isConectUdp)
    {
        mainWindowDepuracionUDP = new MainWindowDepuracionUDP(this);
        mainWindowDepuracionUDP->setUdpSocket(udpSocket, &isConectUdp, &address, &port);
        mainWindowDepuracionUDP->show();
    }

    else
    {
        QMessageBox(QMessageBox::Icon::Warning, "Error al iniciar la depuración", "Primero conectese a una dirección IP",
                    QMessageBox::Button::Ok, this).exec();

        dialogConectarUDP = new DialogConectarUDP(this);
        dialogConectarUDP->setUdpSocket(&isConectUdp, &address, &port);
        dialogConectarUDP->show();
    }
}

void MainWindow::on_actionUSB_2_triggered()
{
    dialogConectarUSB = new DialogConectarUSB(this);
    dialogConectarUSB->setSerialPort(serialPort);
    dialogConectarUSB->show();
}

void MainWindow::on_actionUDP_2_triggered()
{
    dialogConectarUDP = new DialogConectarUDP(this);
    dialogConectarUDP->setUdpSocket(&isConectUdp, &address, &port);
    dialogConectarUDP->show();
}
