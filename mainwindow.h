#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QSerialPort>
#include <QUdpSocket>

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

        QSerialPort *serialPort = nullptr;

        QUdpSocket *udpSocket = nullptr;
        QHostAddress ip;
        quint16 port;

        MainWindowDepuracionUSB *mainWindowDepuracionUSB = nullptr;
        MainWindowDepuracionUDP *mainWindowDepuracionUDP = nullptr;
        DialogConectarUSB *dialogConectarUSB = nullptr;
        DialogConectarUDP *dialogConectarUDP = nullptr;

    private slots:
        void on_actionUSB_triggered();
        void on_actionUDP_triggered();
        void on_actionUSB_2_triggered();
        void on_actionUDP_2_triggered();
};
#endif // MAINWINDOW_H
