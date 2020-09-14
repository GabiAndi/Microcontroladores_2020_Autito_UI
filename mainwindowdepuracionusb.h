#ifndef MAINWINDOWDEPURACIONUSB_H
#define MAINWINDOWDEPURACIONUSB_H

#include <QMainWindow>

#include <QSerialPort>
#include <QMessageBox>

#include <QDebug>

QT_BEGIN_NAMESPACE
namespace Ui {class MainWindowDepuracionUSB;}
QT_END_NAMESPACE

class MainWindowDepuracionUSB : public QMainWindow
{
        Q_OBJECT

    public:
        explicit MainWindowDepuracionUSB(QWidget *parent = nullptr);
        ~MainWindowDepuracionUSB();

        void setSerialPort(QSerialPort *serialPort);

    protected:
        void closeEvent(QCloseEvent *)override;

    private:
        Ui::MainWindowDepuracionUSB *ui;

        QSerialPort *serialPort = nullptr;

        bool captureEnable = false;

        void readData();

    private slots:
        void on_pushButtonLimpiar_clicked();
        void on_pushButtonCapturaDeDatos_clicked();
        void on_pushButtonEnviar_clicked();
        void on_checkBoxCMD_stateChanged(int arg1);
};

#endif // MAINWINDOWDEPURACIONUSB_H
