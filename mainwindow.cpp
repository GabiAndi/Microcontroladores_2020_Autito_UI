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

    if (!log->open(QIODevice::ReadWrite))
    {
        QMessageBox *messageBox = new QMessageBox(this);

        messageBox->setAttribute(Qt::WA_DeleteOnClose);
        messageBox->setIcon(QMessageBox::Icon::Warning);
        messageBox->setWindowTitle("Error al abrir el archivo");
        messageBox->setText("No se pudo crear el archivo del log de sistema");
        messageBox->setStandardButtons(QMessageBox::Button::Ok);

        messageBox->open();
    }

    // Archivo de datos del ADC
    adcData = new QFile();

    // Se crea los graficos
    createChartADC();

    // Timers de integridad de conexion
    // Para UDP
    timerCheckStatusUDP = new QTimer(this);

    connect(timerCheckStatusUDP, &QTimer::timeout, this, &MainWindow::checkStatusUDP);

    timerCheckStatusUDP->start(1000);

    timerPingUDP = new QElapsedTimer();

    timerPingUDP->invalidate();

    // Para USB
    timerCheckStatusUSB = new QTimer(this);

    connect(timerCheckStatusUSB, &QTimer::timeout, this, &MainWindow::checkStatusUSB);

    timerCheckStatusUSB->start(1000);

    timerPingUSB = new QElapsedTimer();

    timerPingUSB->invalidate();
}

MainWindow::~MainWindow()
{
    disconnect(serialPort, &QSerialPort::readyRead, this, &MainWindow::readDataUSB);
    disconnect(udpSocket, &QUdpSocket::readyRead, this, &MainWindow::readDataUDP);

    disconnect(timerUSBReadTimeOut, &QTimer::timeout, this, &MainWindow::timeOutReadUSB);
    disconnect(timerUDPReadTimeOut, &QTimer::timeout, this, &MainWindow::timeOutReadUDP);

    disconnect(timerCheckStatusUDP, &QTimer::timeout, this, &MainWindow::checkStatusUDP);
    disconnect(timerCheckStatusUSB, &QTimer::timeout, this, &MainWindow::checkStatusUSB);

    delete serialPort;
    delete udpSocket;

    delete log;
    delete adcData;

    delete adc0Spline;
    delete adc1Spline;
    delete adc2Spline;
    delete adc3Spline;
    delete adc4Spline;
    delete adc5Spline;
    delete adcChart;
    delete adcChartView;
    delete adcLayout;

    delete timerCheckStatusUDP;
    delete timerPingUDP;
    delete timerCheckStatusUSB;

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *)
{
    if (log->isOpen())
    {
        log->close();
    }

    if (adcData->isOpen())
    {
        adcData->close();
    }
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
                                case 0xD5:
                                    if (buffer_read_usb.data[buffer_read_usb.payload_init] == 0x00)
                                    {
                                        QMessageBox *messageBox = new QMessageBox(this);

                                        messageBox->setAttribute(Qt::WA_DeleteOnClose);
                                        messageBox->setIcon(QMessageBox::Icon::Information);
                                        messageBox->setWindowTitle("Parámetros de conexión");
                                        messageBox->setText("Guardado en FLASH correcto");
                                        messageBox->setStandardButtons(QMessageBox::Button::Ok);

                                        messageBox->open();
                                    }

                                    else
                                    {
                                        QMessageBox *messageBox = new QMessageBox(this);

                                        messageBox->setAttribute(Qt::WA_DeleteOnClose);
                                        messageBox->setIcon(QMessageBox::Icon::Critical);
                                        messageBox->setWindowTitle("Parámetros de conexión");
                                        messageBox->setText("Guardado en FLASH erroneo");
                                        messageBox->setStandardButtons(QMessageBox::Button::Ok);

                                        messageBox->open();
                                    }

                                    break;

                                case 0xC0:  // Datos del ADC
                                    // Sensor 1
                                    byte_translate.u8[0] = buffer_read_usb.data[buffer_read_usb.payload_init + 1];
                                    byte_translate.u8[1] = buffer_read_usb.data[buffer_read_usb.payload_init + 2];

                                    ui->widgetAuto->setSensor1Value(byte_translate.u16[0]);

                                    if (adcData->isOpen())
                                    {
                                        adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                    }

                                    addPointChartADC0(byte_translate.u16[0]);

                                    // Sensor 2
                                    byte_translate.u8[0] = buffer_read_usb.data[buffer_read_usb.payload_init + 3];
                                    byte_translate.u8[1] = buffer_read_usb.data[buffer_read_usb.payload_init + 4];

                                    ui->widgetAuto->setSensor2Value(byte_translate.u16[0]);

                                    if (adcData->isOpen())
                                    {
                                        adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                    }

                                    addPointChartADC1(byte_translate.u16[0]);

                                    // Sensor 3
                                    byte_translate.u8[0] = buffer_read_usb.data[buffer_read_usb.payload_init + 5];
                                    byte_translate.u8[1] = buffer_read_usb.data[buffer_read_usb.payload_init + 6];

                                    ui->widgetAuto->setSensor3Value(byte_translate.u16[0]);

                                    if (adcData->isOpen())
                                    {
                                        adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                    }

                                    addPointChartADC2(byte_translate.u16[0]);

                                    // Sensor 4
                                    byte_translate.u8[0] = buffer_read_usb.data[buffer_read_usb.payload_init + 7];
                                    byte_translate.u8[1] = buffer_read_usb.data[buffer_read_usb.payload_init + 8];

                                    ui->widgetAuto->setSensor4Value(byte_translate.u16[0]);

                                    if (adcData->isOpen())
                                    {
                                        adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                    }

                                    addPointChartADC3(byte_translate.u16[0]);

                                    // Sensor 5
                                    byte_translate.u8[0] = buffer_read_usb.data[buffer_read_usb.payload_init + 9];
                                    byte_translate.u8[1] = buffer_read_usb.data[buffer_read_usb.payload_init + 10];

                                    ui->widgetAuto->setSensor5Value(byte_translate.u16[0]);

                                    if (adcData->isOpen())
                                    {
                                        adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                    }

                                    addPointChartADC4(byte_translate.u16[0]);

                                    // Sensor 6
                                    byte_translate.u8[0] = buffer_read_usb.data[buffer_read_usb.payload_init + 11];
                                    byte_translate.u8[1] = buffer_read_usb.data[buffer_read_usb.payload_init + 12];

                                    ui->widgetAuto->setSensor6Value(byte_translate.u16[0]);

                                    if (adcData->isOpen())
                                    {
                                        adcData->write(QString::asprintf("%u\r\n", byte_translate.u16[0]).toLatin1());
                                    }

                                    addPointChartADC5(byte_translate.u16[0]);

                                    break;

                                case 0xF0:  // ALIVE
                                    pingUSB();

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
                                    case 0xD5:
                                        if (buffer_read_udp.data[buffer_read_udp.payload_init] == 0x00)
                                        {
                                            QMessageBox *messageBox = new QMessageBox(this);

                                            messageBox->setAttribute(Qt::WA_DeleteOnClose);
                                            messageBox->setIcon(QMessageBox::Icon::Information);
                                            messageBox->setWindowTitle("Parámetros de conexión");
                                            messageBox->setText("Guardado en FLASH correcto");
                                            messageBox->setStandardButtons(QMessageBox::Button::Ok);

                                            messageBox->open();
                                        }

                                        else
                                        {
                                            QMessageBox *messageBox = new QMessageBox(this);

                                            messageBox->setAttribute(Qt::WA_DeleteOnClose);
                                            messageBox->setIcon(QMessageBox::Icon::Critical);
                                            messageBox->setWindowTitle("Parámetros de conexión");
                                            messageBox->setText("Guardado en FLASH erroneo");
                                            messageBox->setStandardButtons(QMessageBox::Button::Ok);

                                            messageBox->open();
                                        }

                                        break;

                                    case 0xC0:  // Datos del ADC
                                        // Sensor 1
                                        byte_translate.u8[0] = buffer_read_udp.data[buffer_read_udp.payload_init + 1];
                                        byte_translate.u8[1] = buffer_read_udp.data[buffer_read_udp.payload_init + 2];

                                        ui->widgetAuto->setSensor1Value(byte_translate.u16[0]);

                                        if (adcData->isOpen())
                                        {
                                            adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                        }

                                        addPointChartADC0(byte_translate.u16[0]);

                                        // Sensor 2
                                        byte_translate.u8[0] = buffer_read_udp.data[buffer_read_udp.payload_init + 3];
                                        byte_translate.u8[1] = buffer_read_udp.data[buffer_read_udp.payload_init + 4];

                                        ui->widgetAuto->setSensor2Value(byte_translate.u16[0]);

                                        if (adcData->isOpen())
                                        {
                                            adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                        }

                                        addPointChartADC1(byte_translate.u16[0]);

                                        // Sensor 3
                                        byte_translate.u8[0] = buffer_read_udp.data[buffer_read_udp.payload_init + 5];
                                        byte_translate.u8[1] = buffer_read_udp.data[buffer_read_udp.payload_init + 6];

                                        ui->widgetAuto->setSensor3Value(byte_translate.u16[0]);

                                        if (adcData->isOpen())
                                        {
                                            adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                        }

                                        addPointChartADC2(byte_translate.u16[0]);

                                        // Sensor 4
                                        byte_translate.u8[0] = buffer_read_udp.data[buffer_read_udp.payload_init + 7];
                                        byte_translate.u8[1] = buffer_read_udp.data[buffer_read_udp.payload_init + 8];

                                        ui->widgetAuto->setSensor4Value(byte_translate.u16[0]);

                                        if (adcData->isOpen())
                                        {
                                            adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                        }

                                        addPointChartADC3(byte_translate.u16[0]);

                                        // Sensor 5
                                        byte_translate.u8[0] = buffer_read_udp.data[buffer_read_udp.payload_init + 9];
                                        byte_translate.u8[1] = buffer_read_udp.data[buffer_read_udp.payload_init + 10];

                                        ui->widgetAuto->setSensor5Value(byte_translate.u16[0]);

                                        if (adcData->isOpen())
                                        {
                                            adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                        }

                                        addPointChartADC4(byte_translate.u16[0]);

                                        // Sensor 6
                                        byte_translate.u8[0] = buffer_read_udp.data[buffer_read_udp.payload_init + 11];
                                        byte_translate.u8[1] = buffer_read_udp.data[buffer_read_udp.payload_init + 12];

                                        ui->widgetAuto->setSensor6Value(byte_translate.u16[0]);

                                        if (adcData->isOpen())
                                        {
                                            adcData->write(QString::asprintf("%u\r\n", byte_translate.u16[0]).toLatin1());
                                        }

                                        addPointChartADC5(byte_translate.u16[0]);

                                        break;

                                    case 0xF0:  // ALIVE
                                        pingUDP();

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
        dialogConectarUSB = new DialogConectarUSB(this);
        dialogConectarUSB->setSerialPort(serialPort);
        dialogConectarUSB->show();

        QMessageBox *messageBox = new QMessageBox(this);

        messageBox->setAttribute(Qt::WA_DeleteOnClose);
        messageBox->setIcon(QMessageBox::Icon::Warning);
        messageBox->setWindowTitle("Error al iniciar la depuración");
        messageBox->setText("Primero conectese a un puerto serie");
        messageBox->setStandardButtons(QMessageBox::Button::Ok);

        messageBox->open();
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
        dialogConectarUDP = new DialogConectarUDP(this);
        dialogConectarUDP->setUdpSocket(udpSocket, &ip, &port);
        dialogConectarUDP->show();

        QMessageBox *messageBox = new QMessageBox(this);

        messageBox->setAttribute(Qt::WA_DeleteOnClose);
        messageBox->setIcon(QMessageBox::Icon::Warning);
        messageBox->setWindowTitle("Error al iniciar la depuración");
        messageBox->setText("Primero conectese a una dirección IP");
        messageBox->setStandardButtons(QMessageBox::Button::Ok);

        messageBox->open();
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

void MainWindow::on_pushButtonCapturaDatosADC_clicked()
{
    QByteArray data;

    data.append(0xC0);

    if (ui->pushButtonCapturaDatosADC->text() == "Capturar ADC")
    {
        ui->pushButtonCapturaDatosADC->setText("Detener ADC");

        data.append((uint8_t)(0xFF));
    }

    else if (ui->pushButtonCapturaDatosADC->text() == "Detener ADC")
    {
        ui->pushButtonCapturaDatosADC->setText("Capturar ADC");

        data.append((uint8_t)(0x00));
    }

    data.append((uint8_t)(ui->horizontalSliderTiempoDeCaptura->value()));

    sendCMD(data);
}

void MainWindow::on_horizontalSliderTiempoDeCaptura_valueChanged(int value)
{
    ui->labelTiempoCapturaADC->setText(QString::asprintf("%u ms", value));
}

void MainWindow::on_checkBox_stateChanged(int arg1)
{
    // No esta check
    if (arg1 == 0)
    {
        if (adcData->isOpen())
        {
            adcData->close();
        }
    }

    else
    {
        QString fileName;

        QDate date = QDateTime::currentDateTime().date();
        QTime time = QDateTime::currentDateTime().time();

        if (!QDir("adc").exists())
        {
            QDir().mkdir("adc");
        }

        fileName.append(QString::asprintf("adc/"));

        fileName.append(QString::asprintf("%i", date.year()));

        if (date.month() < 10)
        {
            fileName.append(QString::asprintf("0%i", date.month()));
        }

        else
        {
            fileName.append(QString::asprintf("%i", date.month()));
        }

        if (date.day() < 10)
        {
            fileName.append(QString::asprintf("0%i", date.day()));
        }

        else
        {
            fileName.append(QString::asprintf("%i", date.day()));
        }

        if (time.hour() < 10)
        {
            fileName.append(QString::asprintf("0%i", time.hour()));
        }

        else
        {
            fileName.append(QString::asprintf("%i", time.hour()));
        }

        if (time.minute() < 10)
        {
            fileName.append(QString::asprintf("0%i", time.minute()));
        }

        else
        {
            fileName.append(QString::asprintf("%i", time.minute()));
        }

        if (time.second() < 10)
        {
            fileName.append(QString::asprintf("0%i", time.second()));
        }

        else
        {
            fileName.append(QString::asprintf("%i", time.second()));
        }

        fileName.append(".csv");

        adcData->setFileName(fileName);

        if (adcData->open(QIODevice::ReadWrite))
        {
            adcData->write("ADC0,ADC1,ADC2,ADC3,ADC4,ADC5\r\n");
        }

        else
        {
            QMessageBox *messageBox = new QMessageBox(this);

            messageBox->setAttribute(Qt::WA_DeleteOnClose);
            messageBox->setIcon(QMessageBox::Icon::Warning);
            messageBox->setWindowTitle("Error al abrir el archivo");
            messageBox->setText("No se pudo crear el archivo del adc de sistema");
            messageBox->setStandardButtons(QMessageBox::Button::Ok);

            messageBox->open();
        }
    }
}

void MainWindow::createChartADC()
{
    adcChart = new QChart();

    adcChart->setTitle("Valores del ADC");
    adcChart->legend()->setVisible(true);
    adcChart->setAnimationOptions(QChart::AnimationOption::NoAnimation);

    adcChartView = new QChartView(adcChart);

    adcChartView->setRenderHint(QPainter::Antialiasing);

    adcLayout = new QGridLayout();

    adcLayout->addWidget(adcChartView, 0, 0);

    ui->widgetAdc->setLayout(adcLayout);

    adc0Spline = new QSplineSeries();
    adc1Spline = new QSplineSeries();
    adc2Spline = new QSplineSeries();
    adc3Spline = new QSplineSeries();
    adc4Spline = new QSplineSeries();
    adc5Spline = new QSplineSeries();

    for (int i = 0 ; i <= 30 ; i++)
    {
        adc0Datos.append(QPointF(i, 0));
        adc1Datos.append(QPointF(i, 0));
        adc2Datos.append(QPointF(i, 0));
        adc3Datos.append(QPointF(i, 0));
        adc4Datos.append(QPointF(i, 0));
        adc5Datos.append(QPointF(i, 0));
    }

    adc0Spline->append(adc0Datos);
    adc1Spline->append(adc1Datos);
    adc2Spline->append(adc2Datos);
    adc3Spline->append(adc3Datos);
    adc4Spline->append(adc4Datos);
    adc5Spline->append(adc5Datos);

    adc0Spline->setName("ADC0");
    adc1Spline->setName("ADC1");
    adc2Spline->setName("ADC2");
    adc3Spline->setName("ADC3");
    adc4Spline->setName("ADC4");
    adc5Spline->setName("ADC5");

    adcChart->addSeries(adc0Spline);
    adcChart->addSeries(adc1Spline);
    adcChart->addSeries(adc2Spline);
    adcChart->addSeries(adc3Spline);
    adcChart->addSeries(adc4Spline);
    adcChart->addSeries(adc5Spline);

    adcChart->createDefaultAxes();
    adcChart->axes(Qt::Vertical).first()->setRange(0, 4096);
    adcChart->axes(Qt::Horizontal).first()->setRange(0, 30);
}

void MainWindow::addPointChartADC0(uint16_t point)
{
    for (int i = 0 ; i < 30 ; i++)
    {
        adc0Datos.replace(i, QPointF(i, adc0Datos.value(i + 1).ry()));
    }

    adc0Datos.removeLast();
    adc0Datos.append(QPointF(30, point * 1.0));

    adc0Spline->clear();
    adc0Spline->append(adc0Datos);
}

void MainWindow::addPointChartADC1(uint16_t point)
{
    for (int i = 0 ; i < 30 ; i++)
    {
        adc1Datos.replace(i, QPointF(i, adc1Datos.value(i + 1).ry()));
    }

    adc1Datos.removeLast();
    adc1Datos.append(QPointF(30, point * 1.0));

    adc1Spline->clear();
    adc1Spline->append(adc1Datos);
}

void MainWindow::addPointChartADC2(uint16_t point)
{
    for (int i = 0 ; i < 30 ; i++)
    {
        adc2Datos.replace(i, QPointF(i, adc2Datos.value(i + 1).ry()));
    }

    adc2Datos.removeLast();
    adc2Datos.append(QPointF(30, point * 1.0));

    adc2Spline->clear();
    adc2Spline->append(adc2Datos);
}

void MainWindow::addPointChartADC3(uint16_t point)
{
    for (int i = 0 ; i < 30 ; i++)
    {
        adc3Datos.replace(i, QPointF(i, adc3Datos.value(i + 1).ry()));
    }

    adc3Datos.removeLast();
    adc3Datos.append(QPointF(30, point * 1.0));

    adc3Spline->clear();
    adc3Spline->append(adc3Datos);
}

void MainWindow::addPointChartADC4(uint16_t point)
{
    for (int i = 0 ; i < 30 ; i++)
    {
        adc4Datos.replace(i, QPointF(i, adc4Datos.value(i + 1).ry()));
    }

    adc4Datos.removeLast();
    adc4Datos.append(QPointF(30, point * 1.0));

    adc4Spline->clear();
    adc4Spline->append(adc4Datos);
}

void MainWindow::addPointChartADC5(uint16_t point)
{
    for (int i = 0 ; i < 30 ; i++)
    {
        adc5Datos.replace(i, QPointF(i, adc5Datos.value(i + 1).ry()));
    }

    adc5Datos.removeLast();
    adc5Datos.append(QPointF(30, point * 1.0));

    adc5Spline->clear();
    adc5Spline->append(adc5Datos);
}

void MainWindow::sendCMD(QByteArray sendData, SendTarget Target)
{
    QByteArray data;
    uint8_t checksum = 0;

    data.append('U');
    data.append('N');
    data.append('E');
    data.append('R');
    data.append(sendData.length() - 1);
    data.append(':');

    data.append(sendData);

    for (char byte : data)
    {
        checksum ^= (uint8_t)(byte);
    }

    data.append(checksum);

    switch (Target)
    {
        case SendTarget::SendALL:
            if (serialPort->isOpen())
            {
                serialPort->write(data);
            }

            if (udpSocket->isOpen())
            {
                data.append('\r');
                data.append('\n');

                udpSocket->writeDatagram(data, ip, port);
            }

            break;

        case SendTarget::SendUSB:
            if (serialPort->isOpen())
            {
                serialPort->write(data);
            }

            break;

        case SendTarget::SendUDP:
            if (udpSocket->isOpen())
            {
                data.append('\r');
                data.append('\n');

                udpSocket->writeDatagram(data, ip, port);
            }

            break;
    }
}

void MainWindow::on_pushButtonConfigurarWiFi_clicked()
{
    bool ok;

    QString ssid;
    QString psw;

    QString ipMcu;
    QString ipPc;
    QString port;

    // Para el SSID
    ssid = QInputDialog::getText(this, "Parámetros de conexión", "Escriba el SSID", QLineEdit::EchoMode::Normal, "", &ok,
                           Qt::WindowCloseButtonHint);

    if (ok && !ssid.isEmpty())
    {
        QByteArray data;

        data.append(0xD0);

        data.append(ssid.length());

        data.append(ssid.toLatin1());

        sendCMD(data);
    }

    // Para la contraseña
    psw = QInputDialog::getText(this, "Parámetros de conexión", "Contraseña de " + ssid, QLineEdit::EchoMode::Normal, "", &ok,
                           Qt::WindowCloseButtonHint);

    if (ok && !psw.isEmpty())
    {
        QByteArray data;

        data.append(0xD1);

        data.append(psw.length());

        data.append(psw.toLatin1());

        sendCMD(data);
    }

    // Para la ip del mcu
    ipMcu = QInputDialog::getText(this, "Parámetros de conexión", "IP para el autito", QLineEdit::EchoMode::Normal, "", &ok,
                           Qt::WindowCloseButtonHint);

    if (ok && !ipMcu.isEmpty())
    {
        QByteArray data;

        data.append(0xD2);

        data.append(ipMcu.length());

        data.append(ipMcu.toLatin1());

        sendCMD(data);
    }

    // Para la ip de la pc
    ipPc = QInputDialog::getText(this, "Parámetros de conexión", "IP para la PC", QLineEdit::EchoMode::Normal, "", &ok,
                           Qt::WindowCloseButtonHint);

    if (ok && !ipPc.isEmpty())
    {
        QByteArray data;

        data.append(0xD3);

        data.append(ipPc.length());

        data.append(ipPc.toLatin1());

        sendCMD(data);
    }

    // Puerto de comunicacion
    port = QInputDialog::getText(this, "Parámetros de conexión", "Puerto de comunicación", QLineEdit::EchoMode::Normal, "", &ok,
                           Qt::WindowCloseButtonHint);

    if (ok && !port.isEmpty())
    {
        QByteArray data;

        data.append(0xD4);

        data.append(port.length());

        data.append(port.toLatin1());

        sendCMD(data);
    }

    // Se guardan los datos en la flash del micro
    QMessageBox flashMsg;

    flashMsg.setWindowTitle("Parámetros de conexión");
    flashMsg.setText("¿Desea guardar datos en FLASH?");
    flashMsg.setStandardButtons(QMessageBox::Button::No | QMessageBox::Button::Yes);
    flashMsg.setDefaultButton(QMessageBox::Button::Yes);

    int flash = flashMsg.exec();

    switch (flash)
    {
        case QMessageBox::Button::Yes:
            QByteArray data;

            data.append(0xD5);

            data.append(0xFF);

            sendCMD(data);

            break;
    }
}

void MainWindow::checkStatusUDP()
{
    if (udpSocket->isOpen())
    {
        ui->labelEstadoUDP->setText("Conectado");
        ui->labelIPAutito->setText(ip.toString());
        ui->labelPuerto->setText(QString::number(port));

        if (!timerPingUDP->isValid())
        {
            timerPingUDP->start();
        }

        if (timerPingUDP->elapsed() < 1000)
        {
            QPalette palette = ui->labelLatenciaUDP->palette();

            palette.setColor(ui->labelLatenciaUDP->foregroundRole(), QColor(0x5C, 0xA8, 0x59, 0xFF));

            ui->labelLatenciaUDP->setPalette(palette);
        }

        if (timerPingUDP->elapsed() >= 1000)
        {
            ui->labelLatenciaUDP->setText("<" + QString::number(timerPingUDP->elapsed()) + " ms");

            QPalette palette = ui->labelLatenciaUDP->palette();

            palette.setColor(ui->labelLatenciaUDP->foregroundRole(), QColor(0xA5, 0xC8, 0x00, 0xFF));

            ui->labelLatenciaUDP->setPalette(palette);
        }

        if (timerPingUDP->elapsed() >= 4000)
        {
            ui->labelLatenciaUDP->setText("<" + QString::number(timerPingUDP->elapsed()) + " ms");

            QPalette palette = ui->labelLatenciaUDP->palette();

            palette.setColor(ui->labelLatenciaUDP->foregroundRole(), QColor(0xFF, 0x00, 0x00, 0xFF));

            ui->labelLatenciaUDP->setPalette(palette);
        }

        if (timerPingUDP->elapsed() >= 15000)
        {
            udpSocket->close();
        }

        QByteArray data;

        data.append(0xF0);

        sendCMD(data, SendTarget::SendUDP);
    }

    else
    {
        ui->labelEstadoUDP->setText("Desconectado");

        if (timerPingUDP->isValid())
        {
            timerPingUDP->invalidate();
        }

        ui->labelLatenciaUDP->setText("");
        ui->labelIPAutito->setText("");
        ui->labelPuerto->setText("");
    }
}

void MainWindow::pingUDP()
{
    if (timerPingUDP->isValid())
    {
        ui->labelLatenciaUDP->setText(QString::number(timerPingUDP->restart()) + " ms");

        timerPingUDP->invalidate();
    }
}

void MainWindow::checkStatusUSB()
{
    if (serialPort->isOpen())
    {
        ui->labelEstadoUSB->setText("Conectado");

        if (!timerPingUSB->isValid())
        {
            timerPingUSB->start();
        }

        if (timerPingUSB->elapsed() < 1000)
        {
            QPalette palette = ui->labelLatenciaUSB->palette();

            palette.setColor(ui->labelLatenciaUSB->foregroundRole(), QColor(0x5C, 0xA8, 0x59, 0xFF));

            ui->labelLatenciaUSB->setPalette(palette);
        }

        if (timerPingUSB->elapsed() >= 1000)
        {
            ui->labelLatenciaUSB->setText("<" + QString::number(timerPingUSB->elapsed()) + " ms");

            QPalette palette = ui->labelLatenciaUSB->palette();

            palette.setColor(ui->labelLatenciaUSB->foregroundRole(), QColor(0xA5, 0xC8, 0x00, 0xFF));

            ui->labelLatenciaUSB->setPalette(palette);
        }

        if (timerPingUSB->elapsed() >= 4000)
        {
            ui->labelLatenciaUSB->setText("<" + QString::number(timerPingUSB->elapsed()) + " ms");

            QPalette palette = ui->labelLatenciaUSB->palette();

            palette.setColor(ui->labelLatenciaUSB->foregroundRole(), QColor(0xFF, 0x00, 0x00, 0xFF));

            ui->labelLatenciaUSB->setPalette(palette);
        }

        if (timerPingUSB->elapsed() >= 15000)
        {
            serialPort->close();
        }

        QByteArray data;

        data.append(0xF0);

        sendCMD(data, SendTarget::SendUSB);
    }

    else
    {
        ui->labelEstadoUSB->setText("Desconectado");

        if (timerPingUSB->isValid())
        {
            timerPingUSB->invalidate();
        }

        ui->labelLatenciaUSB->setText("");
    }
}

void MainWindow::pingUSB()
{
    if (timerPingUSB->isValid())
    {
        ui->labelLatenciaUSB->setText(QString::number(timerPingUSB->restart()) + " ms");

        timerPingUSB->invalidate();
    }
}

void MainWindow::on_pushButtonEnviarVelocidadMotor_clicked()
{
    QByteArray data;

    data.append(0xC1);

    byte_translate.f = ui->doubleSpinBoxMotorDerecha->value();

    data.append(byte_translate.u8[0]);
    data.append(byte_translate.u8[1]);
    data.append(byte_translate.u8[2]);
    data.append(byte_translate.u8[3]);

    byte_translate.f = ui->doubleSpinBoxMotorIzquierda->value();

    data.append(byte_translate.u8[0]);
    data.append(byte_translate.u8[1]);
    data.append(byte_translate.u8[2]);
    data.append(byte_translate.u8[3]);

    byte_translate.u16[0] = ui->spinBoxTiempoMotores->value();

    data.append(byte_translate.u8[0]);
    data.append(byte_translate.u8[1]);

    sendCMD(data);
}
