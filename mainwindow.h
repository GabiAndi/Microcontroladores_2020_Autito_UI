#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QSerialPort>
#include <QSerialPortInfo>

#include "dialogmododepuracion.h"

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

        DialogModoDepuracion *dialogModeDepuracion = nullptr;

        QSerialPort *qSerialPort = nullptr;

    private slots:
        void on_actionSalir_triggered();
        void on_actionVentana_de_depuraci_n_triggered();
};
#endif // MAINWINDOW_H
