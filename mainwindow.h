#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QSerialPort>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QTime>
#include <QDate>
#include <QDir>
#include <QtCharts>
#include <QSplineSeries>
#include <QChartView>
#include <QGridLayout>
#include <QList>
#include <QInputDialog>
#include <QDialog>
#include <QElapsedTimer>
#include <QPalette>
#include <QColor>

#include <inttypes.h>

#include "systemmanager.h"
#include "mainwindowdepuracionusb.h"
#include "mainwindowdepuracionudp.h"
#include "dialogconectarusb.h"
#include "dialogconectarudp.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
        Q_OBJECT

    public:
        MainWindow(QWidget *parent = nullptr);
        ~MainWindow();

    protected:
        void closeEvent(QCloseEvent *)override;

    private:
        Ui::MainWindow *ui;

        enum SendTarget
        {
            SendALL = 0,
            SendUSB = 1,
            SendUDP = 2
        };

        typedef union
        {
            uint8_t u8[4];
            uint16_t u16[2];
            uint32_t u32;

            int8_t i8[4];
            int16_t i16[2];
            int32_t i32;

            float f;
        }byte_converter_u;

        typedef struct
        {
            volatile uint8_t data[256];
            volatile uint8_t read_index;
            volatile uint8_t write_index;
        }ring_buffer_t;

        typedef struct
        {
            ring_buffer_t *buffer_read;

            uint8_t read_state;
            uint8_t read_payload_init;
            uint8_t read_payload_length;

            QTimer *timeout;

            SendTarget sendTarget;
        }cmd_manager_t;

        SystemManager *sys = nullptr;

        QSerialPort *serialPort = nullptr;

        QUdpSocket *udpSocket = nullptr;
        QHostAddress ip;
        quint16 port;

        QTimer *timerCheckStatusUDP = nullptr;
        QElapsedTimer *timerPingUDP = nullptr;

        QTimer *timerCheckStatusUSB = nullptr;
        QElapsedTimer *timerPingUSB = nullptr;

        MainWindowDepuracionUSB *mainWindowDepuracionUSB = nullptr;
        MainWindowDepuracionUDP *mainWindowDepuracionUDP = nullptr;
        DialogConectarUSB *dialogConectarUSB = nullptr;
        DialogConectarUDP *dialogConectarUDP = nullptr;

        ring_buffer_t buffer_read_usb;
        ring_buffer_t buffer_read_udp;

        byte_converter_u byte_converter;

        cmd_manager_t cmd_manager_usb;
        cmd_manager_t cmd_manager_udp;

        QFile *adcData = nullptr;
        QFile *pidData = nullptr;

        // Grafico del ADC
        QSplineSeries *adc0Spline;
        QSplineSeries *adc1Spline;
        QSplineSeries *adc2Spline;
        QSplineSeries *adc3Spline;
        QSplineSeries *adc4Spline;
        QSplineSeries *adc5Spline;
        QChart *adcChart;
        QChartView *adcChartView;
        QGridLayout *adcLayout;

        QList<QPointF> adc0Datos;
        QList<QPointF> adc1Datos;
        QList<QPointF> adc2Datos;
        QList<QPointF> adc3Datos;
        QList<QPointF> adc4Datos;
        QList<QPointF> adc5Datos;

        // Grafico del Error
        QSplineSeries *errorSpline;
        QSplineSeries *errorVelSpline;
        QSplineSeries *errorCeroSpline;
        QChart *errorChart;
        QChartView *errorChartView;
        QGridLayout *errorLayout;

        QList<QPointF> errorDatos;
        QList<QPointF> errorVelDatos;
        QList<QPointF> errorCeroDatos;

        // Grafico del PID
        QSplineSeries *pidpSpline;
        QSplineSeries *piddSpline;
        QSplineSeries *pidiSpline;
        QChart *pidChart;
        QChartView *pidChartView;
        QGridLayout *pidLayout;

        QList<QPointF> pidpDatos;
        QList<QPointF> piddDatos;
        QList<QPointF> pidiDatos;

        // Grafico de los motores
        QSplineSeries *motorDerechaSpline;
        QSplineSeries *motorIzquierdaSpline;
        QChart *motorChart;
        QChartView *motorChartView;
        QGridLayout *motorLayout;

        QList<QPointF> motorDerechaDatos;
        QList<QPointF> motorIzquierdaDatos;

        uint8_t checkXor(uint8_t *data, uint8_t init, uint8_t length);
        uint8_t checkXor(QByteArray data);

        void dataPackage(cmd_manager_t *cmd_manager);

        QString getCurrentDataPackage(cmd_manager_t *cmd_manager);

        void createChartADC();
        void createChartError();
        void createChartPID();
        void createChartMotores();

        void addPointChartADC0(uint16_t point);
        void addPointChartADC1(uint16_t point);
        void addPointChartADC2(uint16_t point);
        void addPointChartADC3(uint16_t point);
        void addPointChartADC4(uint16_t point);
        void addPointChartADC5(uint16_t point);

        void addPointChartError(int16_t point);
        void addPointChartErrorVel(int16_t point);

        void addPointChartPIDP(int16_t point);
        void addPointChartPIDD(int16_t point);
        void addPointChartPIDI(int16_t point);

        void addPointChartMotorDerecha(int16_t point);
        void addPointChartMotorIzquierda(int16_t point);

        void sendCMD(QByteArray sendData, SendTarget Target = SendTarget::SendALL);

        void pingUDP();
        void pingUSB();

    public slots:
        void mainWindowDepuracionUSBClose();
        void mainWindowDepuracionUDPClose();

    private slots:
        void readDataUSB();
        void readDataUDP();

        void timeOutReadUSB();
        void timeOutReadUDP();

        void checkStatusUDP();
        void checkStatusUSB();

        void on_actionUSB_triggered();
        void on_actionUDP_triggered();
        void on_actionUSB_2_triggered();
        void on_actionUDP_2_triggered();
        void on_pushButtonCapturaDatosADC_clicked();
        void on_pushButtonConfigurarWiFi_clicked();
        void on_pushButtonEnviarVelocidadMotor_clicked();
        void on_pushButtonEnviarFrecuencia_clicked();
        void on_checkBoxADC_stateChanged(int arg1);
        void on_checkBoxPID_stateChanged(int arg1);
        void on_pushButtonEnviarPIDKP_clicked();
        void on_pushButtonEnviarPIDKD_clicked();
        void on_pushButtonEnviarPIDKI_clicked();
        void on_pushButtonLeerKP_clicked();
        void on_pushButtonLeerKD_clicked();
        void on_pushButtonLeerKI_clicked();
        void on_horizontalSliderTiempoDeCapturaADC_valueChanged(int value);
        void on_pushButtonGuardarEnFLASH_clicked();
        void on_pushButtonControlAutomatico_clicked();
        void on_pushButtonCapturarPID_clicked();
        void on_horizontalSliderTiempoDeCapturaPID_valueChanged(int value);
        void on_pushButtonLeerPesosPonderaciones_clicked();
        void on_pushButtonEstablecerPesosPonderaciones_clicked();
};
#endif // MAINWINDOW_H
