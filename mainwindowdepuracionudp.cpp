#include "mainwindowdepuracionudp.h"
#include "ui_mainwindowdepuracionudp.h"

MainWindowDepuracionUDP::MainWindowDepuracionUDP(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindowDepuracionUDP)
{
    ui->setupUi(this);

    // Comandos
    ui->comboBoxComandos->addItem("F0 | Alive");
}

MainWindowDepuracionUDP::~MainWindowDepuracionUDP()
{
    delete ui;
}

void MainWindowDepuracionUDP::setUdpSocket(QUdpSocket *udpSocket, QHostAddress *ip, quint16 *port)
{
    this->udpSocket = udpSocket;
    this->ip = ip;
    this->port = port;

    connect(udpSocket, &QUdpSocket::readyRead, this, &MainWindowDepuracionUDP::readData);
}

void MainWindowDepuracionUDP::closeEvent(QCloseEvent *)
{
    disconnect(udpSocket, &QUdpSocket::readyRead, this, &MainWindowDepuracionUDP::readData);

    deleteLater();
}

void MainWindowDepuracionUDP::readData()
{
    while (udpSocket->hasPendingDatagrams())
    {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();

        QByteArray dataRead = datagram.data();

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
        if (ui->checkBoxNLCR->isChecked())
        {
            data.append((uint8_t)(ui->plainTextEditPayload->toPlainText().length() + 2));
        }

        else
        {
            data.append((uint8_t)(ui->plainTextEditPayload->toPlainText().length()));
        }
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

        if (ui->checkBoxNLCR->isChecked())
        {
            data.append('\r');
            data.append('\n');
        }
    }

    for (char byte : data)
    {
        checksum ^= (uint8_t)(byte);
    }

    data.append(checksum);

    data.append('\r');
    data.append('\n');

    udpSocket->writeDatagram(data, *ip, *port);
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
