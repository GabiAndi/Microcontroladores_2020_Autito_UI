#ifndef DIALOGCONECTARUDP_H
#define DIALOGCONECTARUDP_H

#include <QDialog>

#include <QUdpSocket>
#include <QHostAddress>

#include "systemmanager.h"

namespace Ui {class DialogConectarUDP;}

class DialogConectarUDP : public QDialog
{
        Q_OBJECT

    public:
        explicit DialogConectarUDP(QWidget *parent = nullptr);
        ~DialogConectarUDP();

        void setSystemManager(SystemManager *sys);
        void setUdpSocket(QUdpSocket *udpSocket, QHostAddress *ip, quint16 *port);

    private:
        Ui::DialogConectarUDP *ui;

        SystemManager *sys = nullptr;

        QUdpSocket *udpSocket = nullptr;
        QHostAddress *ip = nullptr;
        quint16 *port = nullptr;

        void closeEvent(QCloseEvent *)override;

    private slots:
        void on_pushButtonCancelar_clicked();
        void on_pushButtonConectar_clicked();
};

#endif // DIALOGCONECTARUDP_H
