#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Inicialización del gestor del sistema
    sys = new SystemManager();

    // Inicializacion de los archivos
    // Archivo de logs del sistema
    if (!sys->createLogFile())
    {
        QMessageBox *messageBox = new QMessageBox(this);

        messageBox->setAttribute(Qt::WA_DeleteOnClose);
        messageBox->setIcon(QMessageBox::Icon::Warning);
        messageBox->setWindowTitle("Error al abrir el archivo");
        messageBox->setText("No se pudo crear el archivo del log de sistema.");
        messageBox->setStandardButtons(QMessageBox::Button::Ok);

        messageBox->open();
    }

    sys->LOG("Inicio de la aplicación");

    // Archivo de datos del ADC
    adcData = new QFile();

    // Puerto serie
    serialPort = new QSerialPort();
    connect(serialPort, &QSerialPort::readyRead, this, &MainWindow::readDataUSB);

    // Socket UDP
    udpSocket = new QUdpSocket();
    connect(udpSocket, &QUdpSocket::readyRead, this, &MainWindow::readDataUDP);

    // Inicializacion de los buffers
    // Para USB
    buffer_read_usb.read_index = 0;
    buffer_read_usb.write_index = 0;

    // Para UDP
    buffer_read_udp.read_index = 0;
    buffer_read_udp.write_index = 0;

    // Inicializacion de los manejadores de comandos
    // Para USB
    cmd_manager_usb.buffer_read = &buffer_read_usb;
    cmd_manager_usb.read_payload_init = 0;
    cmd_manager_usb.read_payload_length = 0;
    cmd_manager_usb.read_state = 0;
    cmd_manager_usb.sendTarget = SendTarget::SendUSB;

    cmd_manager_usb.timeout = new QTimer(this);
    connect(cmd_manager_usb.timeout, &QTimer::timeout, this, &MainWindow::timeOutReadUSB);

    // Para UDP
    cmd_manager_udp.buffer_read = &buffer_read_udp;
    cmd_manager_udp.read_payload_init = 0;
    cmd_manager_udp.read_payload_length = 0;
    cmd_manager_udp.read_state = 0;
    cmd_manager_udp.sendTarget = SendTarget::SendUDP;

    cmd_manager_udp.timeout = new QTimer(this);
    connect(cmd_manager_udp.timeout, &QTimer::timeout, this, &MainWindow::timeOutReadUDP);

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

    disconnect(cmd_manager_usb.timeout, &QTimer::timeout, this, &MainWindow::timeOutReadUSB);
    disconnect(cmd_manager_udp.timeout, &QTimer::timeout, this, &MainWindow::timeOutReadUDP);

    disconnect(timerCheckStatusUDP, &QTimer::timeout, this, &MainWindow::checkStatusUDP);
    disconnect(timerCheckStatusUSB, &QTimer::timeout, this, &MainWindow::checkStatusUSB);

    delete serialPort;
    delete udpSocket;

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

    delete sys;

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *)
{
    if (adcData->isOpen())
    {
        adcData->close();

        sys->LOG("Se cerro el archivo de datos de adc");
    }

    sys->LOG("Cierre de la apliación");
}

void MainWindow::mainWindowDepuracionUSBClose()
{
    disconnect(mainWindowDepuracionUSB, &MainWindowDepuracionUSB::closeSignal, this, &MainWindow::mainWindowDepuracionUSBClose);

    mainWindowDepuracionUSB = nullptr;

    sys->LOG("Cierre de la depuración USB");
}

void MainWindow::mainWindowDepuracionUDPClose()
{
    disconnect(mainWindowDepuracionUDP, &MainWindowDepuracionUDP::closeSignal, this, &MainWindow::mainWindowDepuracionUDPClose);

    mainWindowDepuracionUDP = nullptr;

    sys->LOG("Cierre de la depuración UDP");
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
            dataPackage(&cmd_manager_usb);

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
                dataPackage(&cmd_manager_udp);

                buffer_read_udp.read_index++;
            }
        }
    }
}

void MainWindow::timeOutReadUSB()
{
    cmd_manager_usb.timeout->stop();

    cmd_manager_usb.read_state = 0;

    // Se guarda el paquete que desencadeno el timeout
    QString datosHex = getCurrentDataPackage(&cmd_manager_usb);

    sys->LOG("TimeOut leyendo paquete via USB:\r\n\tComando: "
            + QString::asprintf("%x\r\n\t", cmd_manager_usb.buffer_read->data[cmd_manager_usb.read_payload_init - 1])
            + "Paquete: " + datosHex + "\r\n\t"
            + "Checksum recivido: " + QString::asprintf("%x\r\n\t", cmd_manager_usb.buffer_read->data[cmd_manager_usb.read_payload_init + cmd_manager_usb.read_payload_length])
            + "Checksum esperado: " + QString::asprintf("%x\r\n\t", (checkXor((uint8_t *)(buffer_read_udp.data), cmd_manager_usb.read_payload_init, cmd_manager_usb.read_payload_length)))
            + "Indice de inicio: " + QString::number((uint8_t)(cmd_manager_usb.buffer_read->read_index - cmd_manager_usb.read_payload_length - 8)) + "\r\n\t"
            + "Indice de fin: " + QString::number((uint8_t)(cmd_manager_usb.buffer_read->read_index)));
}

void MainWindow::timeOutReadUDP()
{
    cmd_manager_udp.timeout->stop();

    cmd_manager_udp.read_state = 0;

    // Se guarda el paquete que desencadeno el timeout
    QString datosHex = getCurrentDataPackage(&cmd_manager_udp);

    sys->LOG("TimeOut leyendo paquete via UDP:\r\n\tComando: "
            + QString::asprintf("%x\r\n\t", cmd_manager_udp.buffer_read->data[cmd_manager_udp.read_payload_init - 1])
            + "Paquete: " + datosHex + "\r\n\t"
            + "Checksum recivido: " + QString::asprintf("%x\r\n\t", cmd_manager_udp.buffer_read->data[cmd_manager_udp.read_payload_init + cmd_manager_udp.read_payload_length])
            + "Checksum esperado: " + QString::asprintf("%x\r\n\t", (checkXor((uint8_t *)(buffer_read_udp.data), cmd_manager_udp.read_payload_init, cmd_manager_udp.read_payload_length)))
            + "Indice de inicio: " + QString::number((uint8_t)(cmd_manager_udp.buffer_read->read_index - cmd_manager_udp.read_payload_length - 8)) + "\r\n\t"
            + "Indice de fin: " + QString::number((uint8_t)(cmd_manager_udp.buffer_read->read_index)));
}

uint8_t MainWindow::checkXor(uint8_t *data, uint8_t init, uint8_t length)
{
    uint8_t val_xor = 0x00;

    for (uint8_t i = 0 ; i < length ; i++)
    {
        val_xor ^= data[(uint8_t)(init + i)];
    }

    return val_xor;
}

uint8_t MainWindow::checkXor(QByteArray data)
{
    uint8_t valXor = 0x00;

    for (uint8_t byte : data)
    {
        valXor ^= byte;
    }

    return valXor;
}

void MainWindow::dataPackage(cmd_manager_t *cmd_manager)
{
    switch (cmd_manager->read_state)
    {
        // Inicio de la cabecera
        case 0:
            if (cmd_manager->buffer_read->data[cmd_manager->buffer_read->read_index] == 'U')
            {
                cmd_manager->timeout->start(100);

                cmd_manager->read_state = 1;
            }

            break;

        case 1:
            if (cmd_manager->buffer_read->data[cmd_manager->buffer_read->read_index] == 'N')
            {
                cmd_manager->read_state = 2;
            }

            else
            {
                cmd_manager->read_state = 0;
            }

            break;

        case 2:
            if (cmd_manager->buffer_read->data[cmd_manager->buffer_read->read_index] == 'E')
            {
                cmd_manager->read_state = 3;
            }

            else
            {
                cmd_manager->read_state = 0;
            }

            break;

        case 3:
            if (cmd_manager->buffer_read->data[cmd_manager->buffer_read->read_index] == 'R')
            {
                cmd_manager->read_state = 4;
            }

            else
            {
                cmd_manager->read_state = 0;
            }

            break;

        // Tamaño del payload
        case 4:
            cmd_manager->read_payload_length = cmd_manager->buffer_read->data[cmd_manager->buffer_read->read_index];

            cmd_manager->read_state = 5;

            break;

        // Byte de token
        case 5:
            if (cmd_manager->buffer_read->data[cmd_manager->buffer_read->read_index] == ':')
            {
                cmd_manager->read_state = 6;
            }

            else
            {
                cmd_manager->read_state = 0;
            }

            break;

        // Comando
        case 6:
            cmd_manager->read_payload_init = cmd_manager->buffer_read->read_index + 1;

            cmd_manager->read_state = 7;

            break;

        // Verificación de datos
        case 7:
            // Se espera que se termine de recibir todos los datos
            if (cmd_manager->buffer_read->read_index == (uint8_t)(cmd_manager->read_payload_init + cmd_manager->read_payload_length))
            {
                // Se comprueba la integridad de datos
                if (checkXor((uint8_t *)(cmd_manager->buffer_read->data),
                        (uint8_t)(cmd_manager->read_payload_init - 7),
                        (uint8_t)(cmd_manager->read_payload_length + 7))
                        == cmd_manager->buffer_read->data[cmd_manager->buffer_read->read_index])
                {
                    // Detengo el timeout
                    cmd_manager->timeout->stop();

                    // El estado se resetea
                    cmd_manager->read_state = 0;

                    // Analisis del comando recibido
                    switch (cmd_manager->buffer_read->data[(uint8_t)(cmd_manager->read_payload_init - 1)])
                    {
                        case 0xC0:  // Datos del ADC
                            // Sensor 1
                            byte_converter.u8[0] = cmd_manager->buffer_read->data[cmd_manager->read_payload_init];
                            byte_converter.u8[1] = cmd_manager->buffer_read->data[(uint8_t)(cmd_manager->read_payload_init + 1)];

                            ui->widgetAuto->setSensor1Value(byte_converter.u16[0]);

                            if (adcData->isOpen())
                            {
                                adcData->write(QString::asprintf("%u,", byte_converter.u16[0]).toLatin1());
                            }

                            addPointChartADC0(byte_converter.u16[0]);

                            // Sensor 2
                            byte_converter.u8[0] = cmd_manager->buffer_read->data[(uint8_t)(cmd_manager->read_payload_init + 2)];
                            byte_converter.u8[1] = cmd_manager->buffer_read->data[(uint8_t)(cmd_manager->read_payload_init + 3)];

                            ui->widgetAuto->setSensor2Value(byte_converter.u16[0]);

                            if (adcData->isOpen())
                            {
                                adcData->write(QString::asprintf("%u,", byte_converter.u16[0]).toLatin1());
                            }

                            addPointChartADC1(byte_converter.u16[0]);

                            // Sensor 3
                            byte_converter.u8[0] = cmd_manager->buffer_read->data[(uint8_t)(cmd_manager->read_payload_init + 4)];
                            byte_converter.u8[1] = cmd_manager->buffer_read->data[(uint8_t)(cmd_manager->read_payload_init + 5)];

                            ui->widgetAuto->setSensor3Value(byte_converter.u16[0]);

                            if (adcData->isOpen())
                            {
                                adcData->write(QString::asprintf("%u,", byte_converter.u16[0]).toLatin1());
                            }

                            addPointChartADC2(byte_converter.u16[0]);

                            // Sensor 4
                            byte_converter.u8[0] = cmd_manager->buffer_read->data[(uint8_t)(cmd_manager->read_payload_init + 6)];
                            byte_converter.u8[1] = cmd_manager->buffer_read->data[(uint8_t)(cmd_manager->read_payload_init + 7)];

                            ui->widgetAuto->setSensor4Value(byte_converter.u16[0]);

                            if (adcData->isOpen())
                            {
                                adcData->write(QString::asprintf("%u,", byte_converter.u16[0]).toLatin1());
                            }

                            addPointChartADC3(byte_converter.u16[0]);

                            // Sensor 5
                            byte_converter.u8[0] = cmd_manager->buffer_read->data[(uint8_t)(cmd_manager->read_payload_init + 8)];
                            byte_converter.u8[1] = cmd_manager->buffer_read->data[(uint8_t)(cmd_manager->read_payload_init + 9)];

                            ui->widgetAuto->setSensor5Value(byte_converter.u16[0]);

                            if (adcData->isOpen())
                            {
                                adcData->write(QString::asprintf("%u,", byte_converter.u16[0]).toLatin1());
                            }

                            addPointChartADC4(byte_converter.u16[0]);

                            // Sensor 6
                            byte_converter.u8[0] = cmd_manager->buffer_read->data[(uint8_t)(cmd_manager->read_payload_init + 10)];
                            byte_converter.u8[1] = cmd_manager->buffer_read->data[(uint8_t)(cmd_manager->read_payload_init + 11)];

                            ui->widgetAuto->setSensor6Value(byte_converter.u16[0]);

                            if (adcData->isOpen())
                            {
                                adcData->write(QString::asprintf("%u\r\n", byte_converter.u16[0]).toLatin1());
                            }

                            addPointChartADC5(byte_converter.u16[0]);

                            break;

                        case 0xC1:

                            break;

                        case 0xC2:

                            break;

                        case 0xD5:
                            if (cmd_manager->buffer_read->data[cmd_manager->read_payload_init] == 0x00)
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

                        // Alive
                        case 0xF0:
                            if (cmd_manager->sendTarget == SendTarget::SendUSB)
                            {
                                pingUSB();
                            }

                            else
                            {
                                pingUDP();
                            }

                            break;

                        // Comando no valido
                        default:
                            sys->LOG("Error de comando en el paquete via UDP");

                            break;
                    }
                }

                // Corrupcion de datos al recibir
                else
                {
                    // Se guarda el paquete con errores
                    QString datosHex = getCurrentDataPackage(cmd_manager);

                    // De donde viene el paquete
                    QString target;

                    if (cmd_manager->sendTarget == SendTarget::SendUSB)
                    {
                        target = "USB";
                    }

                    else
                    {
                        target = "UDP";
                    }

                    sys->LOG("Error de ckecksum en el paquete via " + target + ":\r\n\tComando: "
                            + QString::asprintf("%x\r\n\t", cmd_manager->buffer_read->data[cmd_manager->read_payload_init - 1])
                            + "Paquete: " + datosHex + "\r\n\t"
                            + "Checksum recivido: " + QString::asprintf("%x\r\n\t", cmd_manager->buffer_read->data[cmd_manager->read_payload_init + cmd_manager->read_payload_length])
                            + "Checksum esperado: " + QString::asprintf("%x\r\n\t", (checkXor((uint8_t *)(buffer_read_udp.data), cmd_manager->read_payload_init, cmd_manager->read_payload_length)))
                            + "Indice de inicio: " + QString::number((uint8_t)(cmd_manager->buffer_read->read_index - cmd_manager->read_payload_length - 8)) + "\r\n\t"
                            + "Indice de fin: " + QString::number((uint8_t)(cmd_manager->buffer_read->read_index)));
                }
            }

            break;
    }
}

QString MainWindow::getCurrentDataPackage(cmd_manager_t *cmd_manager)
{
    // Se guarda el paquete con errores
    QString datosHex;

    uint8_t i = cmd_manager->buffer_read->read_index - cmd_manager->read_payload_length - 8;

    while (i != (uint8_t)(cmd_manager->buffer_read->read_index + 1))
    {
        if (cmd_manager->buffer_read->data[i] < 0x10)
        {
            datosHex.append(QString::asprintf("0%x ", cmd_manager->buffer_read->data[i]));
        }

        else
        {
            datosHex.append(QString::asprintf("%x ", cmd_manager->buffer_read->data[i]));
        }

        i++;
    }

    return datosHex;
}

void MainWindow::on_actionUSB_triggered()
{
    if (serialPort->isOpen())
    {
        mainWindowDepuracionUSB = new MainWindowDepuracionUSB(this);
        mainWindowDepuracionUSB->setSystemManager(sys);
        mainWindowDepuracionUSB->setSerialPort(serialPort);
        mainWindowDepuracionUSB->show();

        connect(mainWindowDepuracionUSB, &MainWindowDepuracionUSB::closeSignal, this, &MainWindow::mainWindowDepuracionUSBClose);

        sys->LOG("Depuración USB iniciada:\r\n\tNombre: " + serialPort->portName() + "\r\n\tBaud rate: " + QString::number(serialPort->baudRate()));
    }

    else
    {
        dialogConectarUSB = new DialogConectarUSB(this);
        dialogConectarUSB->setSystemManager(sys);
        dialogConectarUSB->setSerialPort(serialPort);
        dialogConectarUSB->show();

        QMessageBox *messageBox = new QMessageBox(this);

        messageBox->setAttribute(Qt::WA_DeleteOnClose);
        messageBox->setIcon(QMessageBox::Icon::Warning);
        messageBox->setWindowTitle("Error al iniciar la depuración");
        messageBox->setText("Primero conectese a un puerto serie");
        messageBox->setStandardButtons(QMessageBox::Button::Ok);

        messageBox->open();

        sys->LOG("Depuración USB tratando de iniciar pero el puerto no estaba abierto");
    }
}

void MainWindow::on_actionUDP_triggered()
{
    if (udpSocket->isOpen())
    {
        mainWindowDepuracionUDP = new MainWindowDepuracionUDP(this);
        mainWindowDepuracionUDP->setSystemManager(sys);
        mainWindowDepuracionUDP->setUdpSocket(udpSocket, &ip, &port);
        mainWindowDepuracionUDP->show();

        connect(mainWindowDepuracionUDP, &MainWindowDepuracionUDP::closeSignal, this, &MainWindow::mainWindowDepuracionUDPClose);

        sys->LOG("Depuración UDP iniciada:\r\n\tIP: " + ip.toString() + "\r\n\tPuerto: " + QString::number(port));
    }

    else
    {
        dialogConectarUDP = new DialogConectarUDP(this);
        dialogConectarUDP->setSystemManager(sys);
        dialogConectarUDP->setUdpSocket(udpSocket, &ip, &port);
        dialogConectarUDP->show();

        QMessageBox *messageBox = new QMessageBox(this);

        messageBox->setAttribute(Qt::WA_DeleteOnClose);
        messageBox->setIcon(QMessageBox::Icon::Warning);
        messageBox->setWindowTitle("Error al iniciar la depuración");
        messageBox->setText("Primero conectese a una dirección IP");
        messageBox->setStandardButtons(QMessageBox::Button::Ok);

        messageBox->open();

        sys->LOG("Depuración UDP tratando de iniciar pero el socket no estaba abierto");
    }
}

void MainWindow::on_actionUSB_2_triggered()
{
    dialogConectarUSB = new DialogConectarUSB(this);
    dialogConectarUSB->setSystemManager(sys);
    dialogConectarUSB->setSerialPort(serialPort);
    dialogConectarUSB->show();

    sys->LOG("Iniciando conexión USB");
}

void MainWindow::on_actionUDP_2_triggered()
{
    dialogConectarUDP = new DialogConectarUDP(this);
    dialogConectarUDP->setSystemManager(sys);
    dialogConectarUDP->setUdpSocket(udpSocket, &ip, &port);
    dialogConectarUDP->show();

    sys->LOG("Iniciando conexión UDP");
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

            sys->LOG("Se cerro el archivo de datos de adc");
        }
    }

    else
    {
        QString fileName;

        QDate date = QDate::currentDate();
        QTime time = QTime::currentTime();

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
            sys->LOG("Se creo el archivo de datos de adc:\r\n\tNombre: " + fileName);

            adcData->write("ADC0,ADC1,ADC2,ADC3,ADC4,ADC5\r\n");
        }

        else
        {
            sys->LOG("Error al crear el archivo de datos de adc:\r\n\tNombre: " + fileName);

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
    sys->LOG("Iniciando gráficas vacias");

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
                udpSocket->writeDatagram(data, ip, port);
            }

            break;
    }
}

void MainWindow::on_pushButtonConfigurarWiFi_clicked()
{
    sys->LOG("Iniciando configuración de conexión WiFi");

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

        sys->LOG("Envio de SSID");
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

        sys->LOG("Envio de PSW");
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

        sys->LOG("Envio de IP MCU");
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

        sys->LOG("Envio de IP PC");
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

        sys->LOG("Envio de PORT");
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

            sys->LOG("Envio de escritura en la FLASH");
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

            sys->LOG("Se cerro el socket UDP porque el objetivo no respondio mas:\r\n\tIP: " + ip.toString() +
                     "\r\n\tPuerto: " + QString::number(port));
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

            sys->LOG("Se cerro el puerto USB porque el objetivo no respondio mas:\r\n\tNombre: " + serialPort->portName()
                + "\r\n\tBaud rate: " + serialPort->baudRate());
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

    byte_converter.f = ui->doubleSpinBoxMotorDerecha->value();

    data.append(byte_converter.u8[0]);
    data.append(byte_converter.u8[1]);
    data.append(byte_converter.u8[2]);
    data.append(byte_converter.u8[3]);

    byte_converter.f = ui->doubleSpinBoxMotorIzquierda->value();

    data.append(byte_converter.u8[0]);
    data.append(byte_converter.u8[1]);
    data.append(byte_converter.u8[2]);
    data.append(byte_converter.u8[3]);

    byte_converter.u16[0] = ui->spinBoxTiempoMotores->value();

    data.append(byte_converter.u8[0]);
    data.append(byte_converter.u8[1]);

    sendCMD(data);
}

void MainWindow::on_pushButtonEnviarFrecuencia_clicked()
{
    QByteArray data;

    data.append(0xC2);

    data.append(0xFF);

    byte_converter.u16[0] = ui->spinBoxFrecuenciaMotores->value();

    data.append(byte_converter.u8[0]);
    data.append(byte_converter.u8[1]);

    sendCMD(data);
}
