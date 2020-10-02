#ifndef MAINWINDOWDEPURACIONUDP_H
#define MAINWINDOWDEPURACIONUDP_H

#include <QMainWindow>

#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QMessageBox>

#include <QDebug>

namespace Ui {class MainWindowDepuracionUDP;}

class MainWindowDepuracionUDP : public QMainWindow
{
        Q_OBJECT

    public:
        explicit MainWindowDepuracionUDP(QWidget *parent = nullptr);
        ~MainWindowDepuracionUDP();

        void setUdpSocket(QUdpSocket *udpSocket, QHostAddress *ip, quint16 *port);

    protected:
        void closeEvent(QCloseEvent *)override;

    private:
        Ui::MainWindowDepuracionUDP *ui;

        bool captureEnable = false;

        QUdpSocket *udpSocket = nullptr;
        QHostAddress *ip = nullptr;
        quint16 *port = nullptr;

        void readData();

    private slots:
        void on_pushButtonLimpiar_clicked();
        void on_pushButtonCapturaDeDatos_clicked();
        void on_pushButtonEnviar_clicked();
        void on_checkBoxCMD_stateChanged(int arg1);
};

#endif // MAINWINDOWDEPURACIONUDP_H
