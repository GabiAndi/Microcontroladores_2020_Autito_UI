#ifndef MAINWINDOWDEPURACIONSERIE_H
#define MAINWINDOWDEPURACIONSERIE_H

#include <QMainWindow>

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>

#include <QDebug>

namespace Ui {
    class MainWindowDepuracionSerie;
}

class MainWindowDepuracionSerie : public QMainWindow
{
        Q_OBJECT

    public:
        explicit MainWindowDepuracionSerie(QWidget *parent = nullptr);
        ~MainWindowDepuracionSerie();

        void setSerialPort(QSerialPort *serialPort);

    protected:
        void closeEvent(QCloseEvent *)override;

    private:
        Ui::MainWindowDepuracionSerie *ui;

        QSerialPort *qSerialPort = nullptr;

        bool captureEnable = false;

        void readData();

    private slots:
        void on_pushButtonActualizarPuertos_clicked();
        void on_pushButtonConectarDesconectar_clicked();
        void on_pushButtonLimpiar_clicked();
        void on_pushButtonCapturaDeDatos_clicked();
        void on_pushButtonEnviar_clicked();
        void on_checkBoxCMD_stateChanged(int arg1);
};

#endif // MAINWINDOWDEPURACIONSERIE_H
