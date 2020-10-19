#include "mainwindowdepuracionudp.h"
#include "ui_mainwindowdepuracionudp.h"

MainWindowDepuracionUDP::MainWindowDepuracionUDP(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindowDepuracionUDP)
{
    ui->setupUi(this);

    // Comandos
    ui->comboBoxComandos->addItem("C0 | Envio de datos del ADC");
    ui->comboBoxComandos->addItem("C1 | Seteo de velocidad de motores");
    ui->comboBoxComandos->addItem("C2 | Cambio de frecuencia de los motores");
    ui->comboBoxComandos->addItem("C3 | Envio de datos del nivel de bateria");
    ui->comboBoxComandos->addItem("D0 | Setea el SSID");
    ui->comboBoxComandos->addItem("D1 | Setea la contraseña");
    ui->comboBoxComandos->addItem("D2 | Setea la ip del autito");
    ui->comboBoxComandos->addItem("D3 | Setea la ip de la PC terminal");
    ui->comboBoxComandos->addItem("D4 | Setea el puerto de comunicación");
    ui->comboBoxComandos->addItem("D5 | Guarda los datos de la RAM en la FLASH");
    ui->comboBoxComandos->addItem("F0 | Alive");
    ui->comboBoxComandos->addItem("F1 | Modo depuración");
    ui->comboBoxComandos->addItem("F2 | Enviar comando AT");
    ui->comboBoxComandos->addItem("F3 | Enviar datos a ESP");
}

MainWindowDepuracionUDP::~MainWindowDepuracionUDP()
{
    delete ui;
}

void MainWindowDepuracionUDP::closeEvent(QCloseEvent *)
{
    emit(closeSignal());

    deleteLater();
}

void MainWindowDepuracionUDP::setSystemManager(SystemManager *sys)
{
    this->sys = sys;
}

void MainWindowDepuracionUDP::setUdpSocket(QUdpSocket *udpSocket, QHostAddress *ip, quint16 *port)
{
    this->udpSocket = udpSocket;
    this->ip = ip;
    this->port = port;
}

void MainWindowDepuracionUDP::readData(QByteArray dataRead)
{
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

void MainWindowDepuracionUDP::on_pushButtonLimpiar_clicked()
{
    ui->tableWidgetDatosRecibidos->clearContents();

    ui->tableWidgetDatosRecibidos->setRowCount(0);
}

void MainWindowDepuracionUDP::on_pushButtonCapturaDeDatos_clicked()
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

void MainWindowDepuracionUDP::on_pushButtonEnviar_clicked()
{
    QByteArray data;
    QStringList payload;
    uint8_t checksum = 0;

    data.append((uint8_t)('U'));
    data.append((uint8_t)('N'));
    data.append((uint8_t)('E'));
    data.append((uint8_t)('R'));

    if (ui->checkBoxHex->isChecked())
    {
        QString text = ui->plainTextEditPayload->toPlainText();

        text.append(' ');

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

    if (udpSocket->isOpen())
    {
        udpSocket->writeDatagram(data, *ip, *port);
    }
}

void MainWindowDepuracionUDP::on_checkBoxCMD_stateChanged(int arg1)
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
