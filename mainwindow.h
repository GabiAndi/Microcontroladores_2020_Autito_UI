#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QSerialPort>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QTimer>

#include <stdint.h>

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

    private:
        Ui::MainWindow *ui;

        typedef union
        {
            volatile uint8_t u8[4];
            volatile uint16_t u16[2];
            volatile uint32_t u32;

            volatile int8_t i8[4];
            volatile int16_t i16[2];
            volatile int32_t i32;
        }byte_translate_u;

        typedef struct
        {
            volatile uint8_t data[256];
            volatile uint8_t read_index;
            volatile uint8_t write_index;

            volatile uint8_t read_state;
            volatile uint8_t payload_length;
            volatile uint8_t payload_init;
        }buffer_read_usb_t;

        typedef struct
        {
            volatile uint8_t data[256];
            volatile uint8_t read_index;
            volatile uint8_t write_index;
        }buffer_write_usb_t;

        typedef struct
        {
            volatile uint8_t data[256];
            volatile uint8_t read_index;
            volatile uint8_t write_index;

            volatile uint8_t read_state;
            volatile uint8_t payload_length;
            volatile uint8_t payload_init;
        }buffer_read_udp_t;

        typedef struct
        {
            volatile uint8_t data[256];
            volatile uint8_t read_index;
            volatile uint8_t write_index;
        }buffer_write_udp_t;

        QSerialPort *serialPort = nullptr;

        QUdpSocket *udpSocket = nullptr;
        QHostAddress ip;
        quint16 port;

        MainWindowDepuracionUSB *mainWindowDepuracionUSB = nullptr;
        MainWindowDepuracionUDP *mainWindowDepuracionUDP = nullptr;
        DialogConectarUSB *dialogConectarUSB = nullptr;
        DialogConectarUDP *dialogConectarUDP = nullptr;

        buffer_read_usb_t buffer_read_usb;
        buffer_write_usb_t buffer_write_usb;

        buffer_read_udp_t buffer_read_udp;
        buffer_write_udp_t buffer_write_udp;

        byte_translate_u byte_translate;

        QTimer *timerUSBReadTimeOut = nullptr;
        QTimer *timerUDPReadTimeOut = nullptr;

        void readDataUSB();
        void readDataUDP();

        void timeOutReadUSB();
        void timeOutReadUDP();

        uint8_t checkXor(uint8_t cmd, uint8_t *payload, uint8_t payloadInit, uint8_t payloadLength);

    public slots:
        void mainWindowDepuracionUSBClose();
        void mainWindowDepuracionUDPClose();

    private slots:
        void on_actionUSB_triggered();
        void on_actionUDP_triggered();
        void on_actionUSB_2_triggered();
        void on_actionUDP_2_triggered();
};
#endif // MAINWINDOW_H
