#ifndef DIALOGCONECTARUSB_H
#define DIALOGCONECTARUSB_H

#include <QDialog>

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>

#include "systemmanager.h"

namespace Ui {class DialogConectarUSB;}

class DialogConectarUSB : public QDialog
{
        Q_OBJECT

    public:
        explicit DialogConectarUSB(QWidget *parent = nullptr);
        ~DialogConectarUSB();

        void setSystemManager(SystemManager *sys);
        void setSerialPort(QSerialPort *serialPort);

    private:
        Ui::DialogConectarUSB *ui;

        SystemManager *sys = nullptr;

        QSerialPort *serialPort = nullptr;

        void closeEvent(QCloseEvent *)override;

    private slots:
        void on_pushButtonActualizarPuertos_clicked();
        void on_pushButtonCancelar_clicked();
        void on_pushButtonConectar_clicked();
};

#endif // DIALOGCONECTARUSB_H
