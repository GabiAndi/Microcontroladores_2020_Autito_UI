#include "mainwindowdepuracionserie.h"
#include "ui_mainwindowdepuracionserie.h"

MainWindowDepuracionSerie::MainWindowDepuracionSerie(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindowDepuracionSerie)
{
    ui->setupUi(this);

    // Velocidades para el puerto serie
    ui->comboBoxBaudRate->addItem("300");
    ui->comboBoxBaudRate->addItem("1200");
    ui->comboBoxBaudRate->addItem("2400");
    ui->comboBoxBaudRate->addItem("4800");
    ui->comboBoxBaudRate->addItem("9600");
    ui->comboBoxBaudRate->addItem("19200");
    ui->comboBoxBaudRate->addItem("38400");
    ui->comboBoxBaudRate->addItem("57600");
    ui->comboBoxBaudRate->addItem("74880");
    ui->comboBoxBaudRate->addItem("115200");
    ui->comboBoxBaudRate->addItem("230400");
    ui->comboBoxBaudRate->addItem("250000");
    ui->comboBoxBaudRate->addItem("500000");
    ui->comboBoxBaudRate->addItem("1000000");
    ui->comboBoxBaudRate->addItem("2000000");

    // Bits de stop
    ui->comboBoxStopBits->addItem("1");
    ui->comboBoxStopBits->addItem("2");

    // Paridad
    ui->comboBoxParidad->addItem("NoParity");
    ui->comboBoxParidad->addItem("EvenParity");
    ui->comboBoxParidad->addItem("OddParity");
    ui->comboBoxParidad->addItem("SpaceParity");
    ui->comboBoxParidad->addItem("MarkParity");
    ui->comboBoxParidad->addItem("UnknownParity");

    // Comandos
    ui->comboBoxComandos->addItem("F0 | Alive");
    ui->comboBoxComandos->addItem("XX | El primer byte del payload como CMD");
}

MainWindowDepuracionSerie::~MainWindowDepuracionSerie()
{
    delete ui;
}

void MainWindowDepuracionSerie::closeEvent(QCloseEvent *)
{
    disconnect(qSerialPort, &QSerialPort::readyRead, this, &MainWindowDepuracionSerie::readData);
}

void MainWindowDepuracionSerie::readData()
{
    QByteArray dataRead = qSerialPort->readAll();

    if (captureEnable)
    {
        for (uint8_t data : dataRead)
        {
            ui->tableWidgetDatosRecibidos->insertRow(ui->tableWidgetDatosRecibidos->rowCount());

            if (data < 0x10)
            {
                ui->tableWidgetDatosRecibidos->setItem(ui->tableWidgetDatosRecibidos->rowCount() - 1, 0,
                                                       new QTableWidgetItem(QString::asprintf("0%x", data)));
            }

            else
            {
                ui->tableWidgetDatosRecibidos->setItem(ui->tableWidgetDatosRecibidos->rowCount() - 1, 0,
                                                       new QTableWidgetItem(QString::asprintf("%x", data)));
            }

            ui->tableWidgetDatosRecibidos->setItem(ui->tableWidgetDatosRecibidos->rowCount() - 1, 1,
                                                   new QTableWidgetItem(QString::asprintf("%c", data)));
            ui->tableWidgetDatosRecibidos->setItem(ui->tableWidgetDatosRecibidos->rowCount() - 1, 2,
                                                   new QTableWidgetItem(QString::asprintf("%d", data)));
        }
    }
}

void MainWindowDepuracionSerie::setSerialPort(QSerialPort *serialPort)
{
    qSerialPort = serialPort;

    // Si ya esta conectado
    if (qSerialPort->isOpen())
    {
        ui->comboBoxPuertoSerie->addItem(qSerialPort->portName());

        switch (qSerialPort->baudRate())
        {
            case 300:
                ui->comboBoxBaudRate->setCurrentIndex(0);

                break;

            case 1200:
                ui->comboBoxBaudRate->setCurrentIndex(1);

                break;

            case 2400:
                ui->comboBoxBaudRate->setCurrentIndex(2);

                break;

            case 4800:
                ui->comboBoxBaudRate->setCurrentIndex(3);

                break;

            case 9600:
                ui->comboBoxBaudRate->setCurrentIndex(4);

                break;

            case 19200:
                ui->comboBoxBaudRate->setCurrentIndex(5);

                break;

            case 38400:
                ui->comboBoxBaudRate->setCurrentIndex(6);

                break;

            case 57600:
                ui->comboBoxBaudRate->setCurrentIndex(7);

                break;

            case 74880:
                ui->comboBoxBaudRate->setCurrentIndex(8);

                break;

            case 115200:
                ui->comboBoxBaudRate->setCurrentIndex(9);

                break;

            case 230400:
                ui->comboBoxBaudRate->setCurrentIndex(10);

                break;

            case 250000:
                ui->comboBoxBaudRate->setCurrentIndex(11);

                break;

            case 500000:
                ui->comboBoxBaudRate->setCurrentIndex(12);

                break;

            case 1000000:
                ui->comboBoxBaudRate->setCurrentIndex(13);

                break;

            case 2000000:
                ui->comboBoxBaudRate->setCurrentIndex(14);

                break;
        }

        if (qSerialPort->stopBits() == QSerialPort::StopBits::OneStop)
        {
            ui->comboBoxStopBits->setCurrentIndex(0);
        }

        else if (qSerialPort->stopBits() == QSerialPort::StopBits::TwoStop)
        {
            ui->comboBoxStopBits->setCurrentIndex(1);
        }

        if (qSerialPort->parity() == QSerialPort::Parity::NoParity)
        {
            ui->comboBoxStopBits->setCurrentIndex(0);
        }

        else if (qSerialPort->parity() == QSerialPort::Parity::OddParity)
        {
            ui->comboBoxStopBits->setCurrentIndex(1);
        }

        else if (qSerialPort->parity() == QSerialPort::Parity::EvenParity)
        {
            ui->comboBoxStopBits->setCurrentIndex(2);
        }

        else if (qSerialPort->parity() == QSerialPort::Parity::MarkParity)
        {
            ui->comboBoxStopBits->setCurrentIndex(3);
        }

        else if (qSerialPort->parity() == QSerialPort::Parity::SpaceParity)
        {
            ui->comboBoxStopBits->setCurrentIndex(4);
        }

        else if (qSerialPort->parity() == QSerialPort::Parity::UnknownParity)
        {
            ui->comboBoxStopBits->setCurrentIndex(5);
        }

        ui->comboBoxPuertoSerie->setEnabled(false);
        ui->comboBoxBaudRate->setEnabled(false);
        ui->comboBoxStopBits->setEnabled(false);
        ui->comboBoxParidad->setEnabled(false);
        ui->pushButtonActualizarPuertos->setEnabled(false);
        ui->pushButtonEnviar->setEnabled(true);
        ui->pushButtonCapturaDeDatos->setEnabled(true);
        ui->pushButtonConectarDesconectar->setText("Desconectar");

        connect(qSerialPort, &QSerialPort::readyRead, this, &MainWindowDepuracionSerie::readData);
    }
}

void MainWindowDepuracionSerie::on_pushButtonActualizarPuertos_clicked()
{
    ui->comboBoxPuertoSerie->clear();

    for (QSerialPortInfo ports : QSerialPortInfo::availablePorts())
    {
        ui->comboBoxPuertoSerie->addItem(ports.portName());
    }
}

void MainWindowDepuracionSerie::on_pushButtonConectarDesconectar_clicked()
{
    if (qSerialPort->isOpen())
    {
        qSerialPort->close();

        disconnect(qSerialPort, &QSerialPort::readyRead, this, &MainWindowDepuracionSerie::readData);

        ui->comboBoxPuertoSerie->setEnabled(true);
        ui->comboBoxBaudRate->setEnabled(true);
        ui->comboBoxStopBits->setEnabled(true);
        ui->comboBoxParidad->setEnabled(true);
        ui->pushButtonEnviar->setEnabled(false);
        ui->pushButtonCapturaDeDatos->setEnabled(false);
        ui->pushButtonActualizarPuertos->setEnabled(true);
        ui->pushButtonConectarDesconectar->setText("Conectar");

        captureEnable = false;

        ui->pushButtonCapturaDeDatos->setText("Iniciar captura");
    }

    else
    {
        qSerialPort->setPortName(ui->comboBoxPuertoSerie->currentText());
        qSerialPort->setBaudRate(ui->comboBoxBaudRate->currentText().toInt());

        if (ui->comboBoxParidad->currentText() == "NoParity")
        {
            qSerialPort->setParity(QSerialPort::NoParity);
        }

        else if (ui->comboBoxParidad->currentText() == "EvenParity")
        {
            qSerialPort->setParity(QSerialPort::EvenParity);
        }

        else if (ui->comboBoxParidad->currentText() == "OddParity")
        {
            qSerialPort->setParity(QSerialPort::OddParity);
        }

        else if (ui->comboBoxParidad->currentText() == "SpaceParity")
        {
            qSerialPort->setParity(QSerialPort::SpaceParity);
        }

        else if (ui->comboBoxParidad->currentText() == "MarkParity")
        {
            qSerialPort->setParity(QSerialPort::MarkParity);
        }

        else if (ui->comboBoxParidad->currentText() == "UnknownParity")
        {
            qSerialPort->setParity(QSerialPort::UnknownParity);
        }

        if (qSerialPort->open(QIODevice::ReadWrite))
        {
            QMessageBox(QMessageBox::Icon::Information, "Conexión exitosa", "Conectado a " + qSerialPort->portName() + ".",
                        QMessageBox::Button::Ok, this).exec();

            connect(qSerialPort, &QSerialPort::readyRead, this, &MainWindowDepuracionSerie::readData);

            ui->comboBoxPuertoSerie->setEnabled(false);
            ui->comboBoxBaudRate->setEnabled(false);
            ui->comboBoxStopBits->setEnabled(false);
            ui->comboBoxParidad->setEnabled(false);
            ui->pushButtonEnviar->setEnabled(true);
            ui->pushButtonCapturaDeDatos->setEnabled(true);
            ui->pushButtonActualizarPuertos->setEnabled(false);
            ui->pushButtonConectarDesconectar->setText("Desconectar");
        }

        else
        {
            QMessageBox(QMessageBox::Icon::Critical, "Error de conexión", "No se pudo conectar a " + qSerialPort->portName() + ".",
                        QMessageBox::Button::Ok, this).exec();
        }
    }
}

void MainWindowDepuracionSerie::on_pushButtonLimpiar_clicked()
{
    ui->tableWidgetDatosRecibidos->clearContents();

    ui->tableWidgetDatosRecibidos->setRowCount(0);
}

void MainWindowDepuracionSerie::on_pushButtonCapturaDeDatos_clicked()
{
    captureEnable = !captureEnable;

    if (captureEnable)
    {
        ui->pushButtonCapturaDeDatos->setText("Detener captura");
    }

    else
    {
        ui->pushButtonCapturaDeDatos->setText("Iniciar captura");
    }
}

void MainWindowDepuracionSerie::on_pushButtonEnviar_clicked()
{
    QString text = ui->plainTextEditPayload->toPlainText();

    text.append(' ');

    QStringList payload;
    uint16_t spaces = text.count(' ');

    while (spaces > 0)
    {
        if (text.at(0) != ' ')
        {
            payload.append(text.section(' ', 0, 0));
            text.remove(0, text.section(' ', 0, 0).length());
        }

        else
        {
            text.remove(0, 1);

            spaces--;
        }
    }

    QByteArray data;

    uint8_t checksum = 0;

    data.append((uint8_t)('U'));
    data.append((uint8_t)('N'));
    data.append((uint8_t)('E'));
    data.append((uint8_t)('R'));

    if (ui->checkBoxHex->isChecked())
    {
        data.append((uint8_t)(payload.length()));
    }

    else
    {
        data.append((uint8_t)(ui->plainTextEditPayload->toPlainText().length()));
    }

    data.append((uint8_t)(':'));

    if (ui->checkBoxCMD->isChecked())
    {
        if (ui->lineEditCMD->text().length() > 0)
        {
            data.append((uint8_t)(ui->lineEditCMD->text().toUInt(nullptr, 16)));
        }

        else
        {
            QMessageBox(QMessageBox::Icon::Critical, "Error de comando",
                        "Ingrese el comando como dato.",
                        QMessageBox::Button::Ok, this).exec();
        }
    }

    else
    {
        data.append(ui->comboBoxComandos->currentText().left(2).toUInt(nullptr, 16));
    }

    if (ui->checkBoxHex->isChecked())
    {
        for (QString byte : payload)
        {
            data.append((uint8_t)(byte.toUInt(nullptr, 16)));
        }
    }

    else
    {
        for (QChar byte : ui->plainTextEditPayload->toPlainText())
        {
            data.append((uint8_t)(byte.toLatin1()));
        }
    }

    for (char byte : data)
    {
        checksum ^= (uint8_t)(byte);
    }

    data.append(checksum);

    qSerialPort->write(data);
}

void MainWindowDepuracionSerie::on_checkBoxCMD_stateChanged(int arg1)
{
    if (arg1 == 0)
    {
        ui->lineEditCMD->setEnabled(false);
    }

    else
    {
        ui->lineEditCMD->setEnabled(true);
    }
}
