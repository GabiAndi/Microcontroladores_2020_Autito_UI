#ifndef DIALOGMODODEPURACION_H
#define DIALOGMODODEPURACION_H

#include <QDialog>

#include <QSerialPort>

#include "mainwindowdepuracionserie.h"

namespace Ui {
    class DialogModoDepuracion;
}

class DialogModoDepuracion : public QDialog
{
        Q_OBJECT

    public:
        explicit DialogModoDepuracion(QWidget *parent = nullptr);
        ~DialogModoDepuracion();

        void setSerialPort(QSerialPort *serialPort);

    private:
        Ui::DialogModoDepuracion *ui;

        MainWindowDepuracionSerie *mainWindowDepuracionSerie = nullptr;

        QSerialPort *qSerialPort = nullptr;

    private slots:
        void on_pushButtonPuertoSerie_clicked();
};

#endif // DIALOGMODODEPURACION_H
