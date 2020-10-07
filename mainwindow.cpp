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
        QMessageBox(QMessageBox::Icon::Warning, "Error al abrir el archivo", "No se pudo crear el archivo del log de sistema",
                    QMessageBox::Button::Ok, this).exec();
    }

    // Archivo de datos del ADC
    adcData = new QFile();

    // Se crea los graficos
    createChartADC();
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

    delete adc1Spline;
    delete adc2Spline;
    delete adc3Spline;
    delete adc4Spline;
    delete adc5Spline;
    delete adc6Spline;
    delete adcChart;
    delete adcChartView;
    delete adcLayout;

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
                                case 0xC0:  // Datos del ADC
                                    // Sensor 1
                                    byte_translate.u8[0] = buffer_read_usb.data[buffer_read_usb.payload_init + 1];
                                    byte_translate.u8[1] = buffer_read_usb.data[buffer_read_usb.payload_init + 2];

                                    ui->widgetAuto->setSensor1Value(byte_translate.u16[0]);

                                    if (adcData->isOpen())
                                    {
                                        adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                    }

                                    addPointChartADC1(byte_translate.u16[0]);

                                    // Sensor 2
                                    byte_translate.u8[0] = buffer_read_usb.data[buffer_read_usb.payload_init + 3];
                                    byte_translate.u8[1] = buffer_read_usb.data[buffer_read_usb.payload_init + 4];

                                    ui->widgetAuto->setSensor2Value(byte_translate.u16[0]);

                                    if (adcData->isOpen())
                                    {
                                        adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                    }

                                    addPointChartADC2(byte_translate.u16[0]);

                                    // Sensor 3
                                    byte_translate.u8[0] = buffer_read_usb.data[buffer_read_usb.payload_init + 5];
                                    byte_translate.u8[1] = buffer_read_usb.data[buffer_read_usb.payload_init + 6];

                                    ui->widgetAuto->setSensor3Value(byte_translate.u16[0]);

                                    if (adcData->isOpen())
                                    {
                                        adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                    }

                                    addPointChartADC3(byte_translate.u16[0]);

                                    // Sensor 4
                                    byte_translate.u8[0] = buffer_read_usb.data[buffer_read_usb.payload_init + 7];
                                    byte_translate.u8[1] = buffer_read_usb.data[buffer_read_usb.payload_init + 8];

                                    ui->widgetAuto->setSensor4Value(byte_translate.u16[0]);

                                    if (adcData->isOpen())
                                    {
                                        adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                    }

                                    addPointChartADC4(byte_translate.u16[0]);

                                    // Sensor 5
                                    byte_translate.u8[0] = buffer_read_usb.data[buffer_read_usb.payload_init + 9];
                                    byte_translate.u8[1] = buffer_read_usb.data[buffer_read_usb.payload_init + 10];

                                    ui->widgetAuto->setSensor5Value(byte_translate.u16[0]);

                                    if (adcData->isOpen())
                                    {
                                        adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                    }

                                    addPointChartADC5(byte_translate.u16[0]);

                                    // Sensor 6
                                    byte_translate.u8[0] = buffer_read_usb.data[buffer_read_usb.payload_init + 11];
                                    byte_translate.u8[1] = buffer_read_usb.data[buffer_read_usb.payload_init + 12];

                                    ui->widgetAuto->setSensor6Value(byte_translate.u16[0]);

                                    if (adcData->isOpen())
                                    {
                                        adcData->write(QString::asprintf("%u\r\n", byte_translate.u16[0]).toLatin1());
                                    }

                                    addPointChartADC6(byte_translate.u16[0]);

                                    refreshChartADC();

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

                                        if (adcData->isOpen())
                                        {
                                            adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                        }

                                        addPointChartADC1(byte_translate.u16[0]);

                                        // Sensor 2
                                        byte_translate.u8[0] = buffer_read_udp.data[buffer_read_udp.payload_init + 3];
                                        byte_translate.u8[1] = buffer_read_udp.data[buffer_read_udp.payload_init + 4];

                                        ui->widgetAuto->setSensor2Value(byte_translate.u16[0]);

                                        if (adcData->isOpen())
                                        {
                                            adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                        }

                                        addPointChartADC2(byte_translate.u16[0]);

                                        // Sensor 3
                                        byte_translate.u8[0] = buffer_read_udp.data[buffer_read_udp.payload_init + 5];
                                        byte_translate.u8[1] = buffer_read_udp.data[buffer_read_udp.payload_init + 6];

                                        ui->widgetAuto->setSensor3Value(byte_translate.u16[0]);

                                        if (adcData->isOpen())
                                        {
                                            adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                        }

                                        addPointChartADC3(byte_translate.u16[0]);

                                        // Sensor 4
                                        byte_translate.u8[0] = buffer_read_udp.data[buffer_read_udp.payload_init + 7];
                                        byte_translate.u8[1] = buffer_read_udp.data[buffer_read_udp.payload_init + 8];

                                        ui->widgetAuto->setSensor4Value(byte_translate.u16[0]);

                                        if (adcData->isOpen())
                                        {
                                            adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                        }

                                        addPointChartADC4(byte_translate.u16[0]);

                                        // Sensor 5
                                        byte_translate.u8[0] = buffer_read_udp.data[buffer_read_udp.payload_init + 9];
                                        byte_translate.u8[1] = buffer_read_udp.data[buffer_read_udp.payload_init + 10];

                                        ui->widgetAuto->setSensor5Value(byte_translate.u16[0]);

                                        if (adcData->isOpen())
                                        {
                                            adcData->write(QString::asprintf("%u,", byte_translate.u16[0]).toLatin1());
                                        }

                                        addPointChartADC5(byte_translate.u16[0]);

                                        // Sensor 6
                                        byte_translate.u8[0] = buffer_read_udp.data[buffer_read_udp.payload_init + 11];
                                        byte_translate.u8[1] = buffer_read_udp.data[buffer_read_udp.payload_init + 12];

                                        ui->widgetAuto->setSensor6Value(byte_translate.u16[0]);

                                        if (adcData->isOpen())
                                        {
                                            adcData->write(QString::asprintf("%u\r\n", byte_translate.u16[0]).toLatin1());
                                        }

                                        addPointChartADC6(byte_translate.u16[0]);

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

void MainWindow::on_pushButtonCapturaDatosADC_clicked()
{
    QByteArray data;
    uint8_t checksum = 0;

    data.append((uint8_t)('U'));
    data.append((uint8_t)('N'));
    data.append((uint8_t)('E'));
    data.append((uint8_t)('R'));
    data.append((uint8_t)(2));
    data.append((uint8_t)(':'));

    data.append((uint8_t)(0xC0));

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

    for (char byte : data)
    {
        checksum ^= (uint8_t)(byte);
    }

    data.append(checksum);

    if (serialPort->isOpen())
    {
        serialPort->write(data);
    }

    else if (udpSocket->isOpen())
    {
        data.append('\r');
        data.append('\n');

        udpSocket->writeDatagram(data, ip, port);
    }
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
            QMessageBox(QMessageBox::Icon::Warning, "Error al abrir el archivo", "No se pudo crear el archivo del adc de sistema",
                        QMessageBox::Button::Ok, this).exec();
        }
    }
}

void MainWindow::createChartADC()
{
    adc1Spline = new QSplineSeries();
    adc2Spline = new QSplineSeries();
    adc3Spline = new QSplineSeries();
    adc4Spline = new QSplineSeries();
    adc5Spline = new QSplineSeries();
    adc6Spline = new QSplineSeries();

    for (int i = 0 ; i <= 30 ; i++)
    {
        adc1Datos.append(QPointF(i, 0));
        adc2Datos.append(QPointF(i, 0));
        adc3Datos.append(QPointF(i, 0));
        adc4Datos.append(QPointF(i, 0));
        adc5Datos.append(QPointF(i, 0));
        adc6Datos.append(QPointF(i, 0));
    }

    adc1Spline->append(adc1Datos);
    adc2Spline->append(adc2Datos);
    adc3Spline->append(adc3Datos);
    adc4Spline->append(adc4Datos);
    adc5Spline->append(adc5Datos);
    adc6Spline->append(adc6Datos);

    adcChart = new QChart();

    adcChart->setTitle("Valores del ADC");
    adcChart->legend()->hide();

    adcChart->addSeries(adc1Spline);
    adcChart->addSeries(adc2Spline);
    adcChart->addSeries(adc3Spline);
    adcChart->addSeries(adc4Spline);
    adcChart->addSeries(adc5Spline);
    adcChart->addSeries(adc6Spline);

    adcChart->createDefaultAxes();
    adcChart->axes(Qt::Vertical).first()->setRange(0, 4096);
    adcChart->axes(Qt::Horizontal).first()->setRange(0, 30);

    adcChartView = new QChartView(adcChart);

    adcChartView->setRenderHint(QPainter::Antialiasing);

    adcLayout = new QGridLayout();

    adcLayout->addWidget(adcChartView, 0, 0);

    ui->widgetAdc->setLayout(adcLayout);
}

void MainWindow::refreshChartADC()
{
    // Se borra la memoria asignada
    delete adc1Spline;
    delete adc2Spline;
    delete adc3Spline;
    delete adc4Spline;
    delete adc5Spline;
    delete adc6Spline;
    delete adcChart;
    delete adcChartView;
    delete adcLayout;

    // Se añade el dato recibido
    adc1Spline = new QSplineSeries();
    adc2Spline = new QSplineSeries();
    adc3Spline = new QSplineSeries();
    adc4Spline = new QSplineSeries();
    adc5Spline = new QSplineSeries();
    adc6Spline = new QSplineSeries();

    adc1Spline->append(adc1Datos);
    adc2Spline->append(adc2Datos);
    adc3Spline->append(adc3Datos);
    adc4Spline->append(adc4Datos);
    adc5Spline->append(adc5Datos);
    adc6Spline->append(adc6Datos);

    adcChart = new QChart();

    adcChart->setTitle("Valores del ADC");
    adcChart->legend()->hide();

    adcChart->addSeries(adc1Spline);
    adcChart->addSeries(adc2Spline);
    adcChart->addSeries(adc3Spline);
    adcChart->addSeries(adc4Spline);
    adcChart->addSeries(adc5Spline);
    adcChart->addSeries(adc6Spline);

    adcChart->createDefaultAxes();
    adcChart->axes(Qt::Vertical).first()->setRange(0, 4096);
    adcChart->axes(Qt::Horizontal).first()->setRange(0, 30);

    adcChartView = new QChartView(adcChart);

    adcChartView->setRenderHint(QPainter::Antialiasing);

    adcLayout = new QGridLayout();

    adcLayout->addWidget(adcChartView, 0, 0);

    ui->widgetAdc->setLayout(adcLayout);
}

void MainWindow::addPointChartADC1(uint16_t point)
{
    for (int i = 0 ; i < 30 ; i++)
    {
        adc1Datos.replace(i, QPointF(i, adc1Datos.value(i + 1).ry()));
    }

    adc1Datos.removeLast();
    adc1Datos.append(QPointF(30, point * 1.0));
}

void MainWindow::addPointChartADC2(uint16_t point)
{
    for (int i = 0 ; i < 30 ; i++)
    {
        adc2Datos.replace(i, QPointF(i, adc2Datos.value(i + 1).ry()));
    }

    adc2Datos.removeLast();
    adc2Datos.append(QPointF(30, point * 1.0));
}

void MainWindow::addPointChartADC3(uint16_t point)
{
    for (int i = 0 ; i < 30 ; i++)
    {
        adc3Datos.replace(i, QPointF(i, adc3Datos.value(i + 1).ry()));
    }

    adc3Datos.removeLast();
    adc3Datos.append(QPointF(30, point * 1.0));
}

void MainWindow::addPointChartADC4(uint16_t point)
{
    for (int i = 0 ; i < 30 ; i++)
    {
        adc4Datos.replace(i, QPointF(i, adc4Datos.value(i + 1).ry()));
    }

    adc4Datos.removeLast();
    adc4Datos.append(QPointF(30, point * 1.0));
}

void MainWindow::addPointChartADC5(uint16_t point)
{
    for (int i = 0 ; i < 30 ; i++)
    {
        adc5Datos.replace(i, QPointF(i, adc5Datos.value(i + 1).ry()));
    }

    adc5Datos.removeLast();
    adc5Datos.append(QPointF(30, point * 1.0));
}

void MainWindow::addPointChartADC6(uint16_t point)
{
    for (int i = 0 ; i < 30 ; i++)
    {
        adc6Datos.replace(i, QPointF(i, adc6Datos.value(i + 1).ry()));
    }

    adc6Datos.removeLast();
    adc6Datos.append(QPointF(30, point * 1.0));
}
