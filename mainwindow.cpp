#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    serialPort = new QSerialPort();
    connect(serialPort, &QSerialPort::readyRead, this, &MainWindow::readDataUSB);

    udpSocket = new QUdpSocket();
    connect(udpSocket, &QUdpSocket::readyRead, this, &MainWindow::readDataUDP);

    // Inicializacion de los buffers
    buffer_read_usb.read_index = 0;
    buffer_read_usb.write_index = 0;
    buffer_read_usb.read_state = 0;

    buffer_write_usb.read_index = 0;
    buffer_write_usb.write_index = 0;

    buffer_read_udp.read_index = 0;
    buffer_read_udp.write_index = 0;
    buffer_read_udp.read_state = 0;

    buffer_write_udp.read_index = 0;
    buffer_write_udp.write_index = 0;

    // Inicializacion de los time out
    timerUSBReadTimeOut = new QTimer(this);
    connect(timerUSBReadTimeOut, &QTimer::timeout, this, &MainWindow::timeOutReadUSB);

    timerUDPReadTimeOut = new QTimer(this);
    connect(timerUDPReadTimeOut, &QTimer::timeout, this, &MainWindow::timeOutReadUDP);

    // Inicializacion de los archivos
    // Archivo de logs del sistema
    log = new QFile("log.txt");
    log->open(QIODevice::ReadWrite);

    // Archivo de datos del ADC
    adcData = new QFile("adc.csv");
    adcData->open(QIODevice::ReadWrite);
    adcData->write("ADC0,ADC1,ADC2,ADC3,ADC4,ADC5\r\n");
}

MainWindow::~MainWindow()
{
    disconnect(serialPort, &QSerialPort::readyRead, this, &MainWindow::readDataUSB);
    disconnect(udpSocket, &QUdpSocket::readyRead, this, &MainWindow::readDataUDP);

    disconnect(timerUSBReadTimeOut, &QTimer::timeout, this, &MainWindow::timeOutReadUSB);
    disconnect(timerUDPReadTimeOut, &QTimer::timeout, this, &MainWindow::timeOutReadUDP);

    delete serialPort;
    delete udpSocket;

    delete log;
    delete adcData;

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *)
{
    log->close();
    adcData->close();
}

void MainWindow::mainWindowDepuracionUSBClose()
{
    disconnect(mainWindowDepuracionUSB, &MainWindowDepuracionUSB::closeSignal, this, &MainWindow::mainWindowDepuracionUSBClose);

    mainWindowDepuracionUSB = nullptr;
}

void MainWindow::mainWindowDepuracionUDPClose()
{
    disconnect(mainWindowDepuracionUDP, &MainWindowDepuracionUDP::closeSignal, this, &MainWindow::mainWindowDepuracionUDPClose);

    mainWindowDepuracionUDP = nullptr;
}

void MainWindow::readDataUSB()
{
    QByteArray dataRead = serialPort->readAll();

    if (mainWindowDepuracionUSB != nullptr)
    {
        mainWindowDepuracionUSB->readData(dataRead);
    }

    for (uint8_t data : dataRead)
    {
        buffer_read_usb.data[buffer_read_usb.write_index] = data;
        buffer_read_usb.write_index++;

        if (buffer_read_usb.read_index != buffer_read_usb.write_index)
        {
            switch (buffer_read_usb.read_state)
            {
                case 0:	// Inicio de la abecera
                    if (buffer_read_usb.data[buffer_read_usb.read_index] == 'U')
                    {
                        buffer_read_usb.read_state = 1;

                        timerUSBReadTimeOut->start(100);
                    }

                    break;

                case 1:
                    if (buffer_read_usb.data[buffer_read_usb.read_index] == 'N')
                    {
                        buffer_read_usb.read_state = 2;
                    }

                    else
                    {
                        buffer_read_usb.read_state = 0;
                    }

                    break;

                case 2:
                    if (buffer_read_usb.data[buffer_read_usb.read_index] == 'E')
                    {
                        buffer_read_usb.read_state = 3;
                    }

                    else
                    {
                        buffer_read_usb.read_state = 0;
                    }

                    break;

                case 3:
                    if (buffer_read_usb.data[buffer_read_usb.read_index] == 'R')
                    {
                        buffer_read_usb.read_state = 4;
                    }

                    else
                    {
                        buffer_read_usb.read_state = 0;
                    }

                    break;

                case 4:	// Lee el tamaño del payload
                    buffer_read_usb.payload_length = buffer_read_usb.data[buffer_read_usb.read_index];

                    buffer_read_usb.read_state = 5;

                    break;

                case 5:	// Token
                    if (buffer_read_usb.data[buffer_read_usb.read_index] == ':')
                    {
                        buffer_read_usb.read_state = 6;
                    }

                    else
                    {
                        buffer_read_usb.read_state = 0;
                    }

                    break;

                case 6:	// Comando
                    buffer_read_usb.payload_init = buffer_read_usb.read_index + 1;

                    buffer_read_usb.read_state = 7;

                    break;

                case 7:	// Verificación de datos
                    // Si se terminaron de recibir todos los datos
                    if (buffer_read_usb.read_index == (buffer_read_usb.payload_init + buffer_read_usb.payload_length))
                    {
                        // Se comprueba la integridad de datos
                        if (checkXor(buffer_read_usb.data[buffer_read_usb.payload_init - 1], (uint8_t *)(buffer_read_usb.data),
                                buffer_read_usb.payload_init, buffer_read_usb.payload_length)
                                == buffer_read_usb.data[buffer_read_usb.read_index])
                        {
                            // Analisis del comando recibido
                            switch (buffer_read_usb.data[buffer_read_usb.payload_init - 1])
                            {
                                case 0xC0:  // Datos del ADC
                                    // Sensor 1
                                    byte_translate.u8[0] = buffer_read_usb.data[buffer_read_usb.payload_init + 1];
                                    byte_translate.u8[1] = buffer_read_usb.data[buffer_read_usb.payload_init + 2];

                                    ui->widgetAuto->setSensor1Value(byte_translate.u16[0]);

                                    adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());

                                    // Sensor 2
                                    byte_translate.u8[0] = buffer_read_usb.data[buffer_read_usb.payload_init + 3];
                                    byte_translate.u8[1] = buffer_read_usb.data[buffer_read_usb.payload_init + 4];

                                    ui->widgetAuto->setSensor2Value(byte_translate.u16[0]);

                                    adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());

                                    // Sensor 3
                                    byte_translate.u8[0] = buffer_read_usb.data[buffer_read_usb.payload_init + 5];
                                    byte_translate.u8[1] = buffer_read_usb.data[buffer_read_usb.payload_init + 6];

                                    ui->widgetAuto->setSensor3Value(byte_translate.u16[0]);

                                    adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());

                                    // Sensor 4
                                    byte_translate.u8[0] = buffer_read_usb.data[buffer_read_usb.payload_init + 7];
                                    byte_translate.u8[1] = buffer_read_usb.data[buffer_read_usb.payload_init + 8];

                                    ui->widgetAuto->setSensor4Value(byte_translate.u16[0]);

                                    adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());

                                    // Sensor 5
                                    byte_translate.u8[0] = buffer_read_usb.data[buffer_read_usb.payload_init + 9];
                                    byte_translate.u8[1] = buffer_read_usb.data[buffer_read_usb.payload_init + 10];

                                    ui->widgetAuto->setSensor5Value(byte_translate.u16[0]);

                                    adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());

                                    // Sensor 6
                                    byte_translate.u8[0] = buffer_read_usb.data[buffer_read_usb.payload_init + 11];
                                    byte_translate.u8[1] = buffer_read_usb.data[buffer_read_usb.payload_init + 12];

                                    ui->widgetAuto->setSensor6Value(byte_translate.u16[0]);

                                    adcData->write(QString::asprintf("%u\r\n", byte_translate.u16[0]).toLatin1());

                                    break;

                                case 0xF0:  // ALIVE

                                    break;

                                default:	// Comando no valido

                                    break;
                            }
                        }

                        // Corrupcion de datos al recibir
                        else
                        {

                        }

                        // Detengo el timeout
                        timerUSBReadTimeOut->stop();

                        buffer_read_usb.read_state = 0;
                    }

                    break;
            }

            buffer_read_usb.read_index++;
        }
    }
}

void MainWindow::readDataUDP()
{
    while (udpSocket->hasPendingDatagrams())
    {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();

        QByteArray dataRead = datagram.data();

        if (mainWindowDepuracionUDP != nullptr)
        {
            mainWindowDepuracionUDP->readData(dataRead);
        }

        for (uint8_t data : dataRead)
        {
            buffer_read_udp.data[buffer_read_udp.write_index] = data;
            buffer_read_udp.write_index++;

            if (buffer_read_udp.read_index != buffer_read_udp.write_index)
            {
                switch (buffer_read_udp.read_state)
                {
                    case 0:	// Inicio de la abecera
                        if (buffer_read_udp.data[buffer_read_udp.read_index] == 'U')
                        {
                            buffer_read_udp.read_state = 1;

                            timerUDPReadTimeOut->start(100);
                        }

                        break;

                    case 1:
                        if (buffer_read_udp.data[buffer_read_udp.read_index] == 'N')
                        {
                            buffer_read_udp.read_state = 2;
                        }

                        else
                        {
                            buffer_read_udp.read_state = 0;
                        }

                        break;

                    case 2:
                        if (buffer_read_udp.data[buffer_read_udp.read_index] == 'E')
                        {
                            buffer_read_udp.read_state = 3;
                        }

                        else
                        {
                            buffer_read_udp.read_state = 0;
                        }

                        break;

                    case 3:
                        if (buffer_read_udp.data[buffer_read_udp.read_index] == 'R')
                        {
                            buffer_read_udp.read_state = 4;
                        }

                        else
                        {
                            buffer_read_udp.read_state = 0;
                        }

                        break;

                    case 4:	// Lee el tamaño del payload
                        buffer_read_udp.payload_length = buffer_read_udp.data[buffer_read_udp.read_index];

                        buffer_read_udp.read_state = 5;

                        break;

                    case 5:	// Token
                        if (buffer_read_udp.data[buffer_read_udp.read_index] == ':')
                        {
                            buffer_read_udp.read_state = 6;
                        }

                        else
                        {
                            buffer_read_udp.read_state = 0;
                        }

                        break;

                    case 6:	// Comando
                        buffer_read_udp.payload_init = buffer_read_udp.read_index + 1;

                        buffer_read_udp.read_state = 7;

                        break;

                    case 7:	// Verificación de datos
                        // Si se terminaron de recibir todos los datos
                        if (buffer_read_udp.read_index == (buffer_read_udp.payload_init + buffer_read_udp.payload_length))
                        {
                            // Se comprueba la integridad de datos
                            if (checkXor(buffer_read_udp.data[buffer_read_udp.payload_init - 1], (uint8_t *)(buffer_read_udp.data),
                                    buffer_read_udp.payload_init, buffer_read_udp.payload_length)
                                    == buffer_read_udp.data[buffer_read_udp.read_index])
                            {
                                // Analisis del comando recibido
                                switch (buffer_read_udp.data[buffer_read_udp.payload_init - 1])
                                {
                                    case 0xC0:  // Datos del ADC
                                        // Sensor 1
                                        byte_translate.u8[0] = buffer_read_udp.data[buffer_read_udp.payload_init + 1];
                                        byte_translate.u8[1] = buffer_read_udp.data[buffer_read_udp.payload_init + 2];

                                        ui->widgetAuto->setSensor1Value(byte_translate.u16[0]);

                                        adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());

                                        // Sensor 2
                                        byte_translate.u8[0] = buffer_read_udp.data[buffer_read_udp.payload_init + 3];
                                        byte_translate.u8[1] = buffer_read_udp.data[buffer_read_udp.payload_init + 4];

                                        ui->widgetAuto->setSensor2Value(byte_translate.u16[0]);

                                        adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());

                                        // Sensor 3
                                        byte_translate.u8[0] = buffer_read_udp.data[buffer_read_udp.payload_init + 5];
                                        byte_translate.u8[1] = buffer_read_udp.data[buffer_read_udp.payload_init + 6];

                                        ui->widgetAuto->setSensor3Value(byte_translate.u16[0]);

                                        adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());

                                        // Sensor 4
                                        byte_translate.u8[0] = buffer_read_udp.data[buffer_read_udp.payload_init + 7];
                                        byte_translate.u8[1] = buffer_read_udp.data[buffer_read_udp.payload_init + 8];

                                        ui->widgetAuto->setSensor4Value(byte_translate.u16[0]);

                                        adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());

                                        // Sensor 5
                                        byte_translate.u8[0] = buffer_read_udp.data[buffer_read_udp.payload_init + 9];
                                        byte_translate.u8[1] = buffer_read_udp.data[buffer_read_udp.payload_init + 10];

                                        ui->widgetAuto->setSensor5Value(byte_translate.u16[0]);

                                        adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());

                                        // Sensor 6
                                        byte_translate.u8[0] = buffer_read_udp.data[buffer_read_udp.payload_init + 11];
                                        byte_translate.u8[1] = buffer_read_udp.data[buffer_read_udp.payload_init + 12];

                                        ui->widgetAuto->setSensor6Value(byte_translate.u16[0]);

                                        adcData->write(QString::asprintf("%u\r\n", byte_translate.u16[0]).toLatin1());

                                        break;

                                    case 0xF0:  // ALIVE

                                        break;

                                    default:	// Comando no valido

                                        break;
                                }
                            }

                            // Corrupcion de datos al recibir
                            else
                            {

                            }

                            // Detengo el timeout
                            timerUDPReadTimeOut->stop();

                            buffer_read_udp.read_state = 0;
                        }

                        break;
                }

                buffer_read_udp.read_index++;
            }
        }
    }
}

void MainWindow::timeOutReadUSB()
{
    timerUSBReadTimeOut->stop();

    buffer_read_usb.read_state = 0;
}

void MainWindow::timeOutReadUDP()
{
    timerUDPReadTimeOut->stop();

    buffer_read_udp.read_state = 0;
}

uint8_t MainWindow::checkXor(uint8_t cmd, uint8_t *payload, uint8_t payloadInit, uint8_t payloadLength)
{
    uint8_t valXor = 0x00;

    valXor ^= 'U';
    valXor ^= 'N';
    valXor ^= 'E';
    valXor ^= 'R';
    valXor ^= payloadLength;
    valXor ^= ':';

    valXor ^= cmd;

    for (uint8_t i = payloadInit ; i < (uint8_t)(payloadInit + payloadLength) ; i++)
    {
        valXor ^= payload[i];
    }

    return valXor;
}

void MainWindow::on_actionUSB_triggered()
{
    if (serialPort->isOpen())
    {
        mainWindowDepuracionUSB = new MainWindowDepuracionUSB(this);
        mainWindowDepuracionUSB->setSerialPort(serialPort);
        mainWindowDepuracionUSB->show();

        connect(mainWindowDepuracionUSB, &MainWindowDepuracionUSB::closeSignal, this, &MainWindow::mainWindowDepuracionUSBClose);
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
    if (udpSocket->isOpen())
    {
        mainWindowDepuracionUDP = new MainWindowDepuracionUDP(this);
        mainWindowDepuracionUDP->setUdpSocket(udpSocket, &ip, &port);
        mainWindowDepuracionUDP->show();

        connect(mainWindowDepuracionUDP, &MainWindowDepuracionUDP::closeSignal, this, &MainWindow::mainWindowDepuracionUDPClose);
    }

    else
    {
        QMessageBox(QMessageBox::Icon::Warning, "Error al iniciar la depuración", "Primero conectese a una dirección IP",
                    QMessageBox::Button::Ok, this).exec();

        dialogConectarUDP = new DialogConectarUDP(this);
        dialogConectarUDP->setUdpSocket(udpSocket, &ip, &port);
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
    dialogConectarUDP->setUdpSocket(udpSocket, &ip, &port);
    dialogConectarUDP->show();
}
