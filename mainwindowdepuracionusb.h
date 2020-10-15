#ifndef MAINWINDOWDEPURACIONUSB_H
#define MAINWINDOWDEPURACIONUSB_H

#include <QMainWindow>

#include <QSerialPort>
#include <QMessageBox>

#include "systemmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui {class MainWindowDepuracionUSB;}
QT_END_NAMESPACE

class MainWindowDepuracionUSB : public QMainWindow
{
        Q_OBJECT

    public:
        explicit MainWindowDepuracionUSB(QWidget *parent = nullptr);
        ~MainWindowDepuracionUSB();

        void setSystemManager(SystemManager *sys);

        void setSerialPort(QSerialPort *serialPort);

        void readData(QByteArray dataRead);

    protected:
        void closeEvent(QCloseEvent *)override;

    private:
        Ui::MainWindowDepuracionUSB *ui;

        SystemManager *sys = nullptr;

        QSerialPort *serialPort = nullptr;

        bool captureEnable = false;

    signals:
        void closeSignal();

    private slots:
        void on_pushButtonLimpiar_clicked();
        void on_pushButtonCapturaDeDatos_clicked();
        void on_pushButtonEnviar_clicked();
        void on_checkBoxCMD_stateChanged(int arg1);
};

#endif // MAINWINDOWDEPURACIONUSB_H
