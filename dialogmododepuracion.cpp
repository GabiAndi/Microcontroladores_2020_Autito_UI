#include "dialogmododepuracion.h"
#include "ui_dialogmododepuracion.h"

DialogModoDepuracion::DialogModoDepuracion(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogModoDepuracion)
{
    ui->setupUi(this);
}

DialogModoDepuracion::~DialogModoDepuracion()
{
    delete ui;
}

void DialogModoDepuracion::setSerialPort(QSerialPort *serialPort)
{
    qSerialPort = serialPort;
}

void DialogModoDepuracion::on_pushButtonPuertoSerie_clicked()
{
    close();

    mainWindowDepuracionSerie = new MainWindowDepuracionSerie(this);

    mainWindowDepuracionSerie->setSerialPort(qSerialPort);

    mainWindowDepuracionSerie->show();
}
