#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    qSerialPort = new QSerialPort();
}

MainWindow::~MainWindow()
{
    delete qSerialPort;

    delete ui;
}


void MainWindow::on_actionSalir_triggered()
{
    exit(0);
}

void MainWindow::on_actionVentana_de_depuraci_n_triggered()
{
    dialogModeDepuracion = new DialogModoDepuracion(this);

    dialogModeDepuracion->setSerialPort(qSerialPort);

    dialogModeDepuracion->show();
}
