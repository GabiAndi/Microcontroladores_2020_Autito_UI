#ifndef DIALOGCONECTARUSB_H
#define DIALOGCONECTARUSB_H

#include <QDialog>

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>

namespace Ui {class DialogConectarUSB;}

class DialogConectarUSB : public QDialog
{
        Q_OBJECT

    public:
        explicit DialogConectarUSB(QWidget *parent = nullptr);
        ~DialogConectarUSB();

        void setSerialPort(QSerialPort *serialPort);

    private:
        Ui::DialogConectarUSB *ui;

        QSerialPort *serialPort = nullptr;

        void closeEvent(QCloseEvent *)override;

    private slots:
        void on_pushButtonActualizarPuertos_clicked();
        void on_pushButtonCancelar_clicked();
        void on_pushButtonConectar_clicked();
};

#endif // DIALOGCONECTARUSB_H
