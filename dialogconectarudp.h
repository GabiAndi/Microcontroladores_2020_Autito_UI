#ifndef DIALOGCONECTARUDP_H
#define DIALOGCONECTARUDP_H

#include <QDialog>

#include <QHostAddress>

namespace Ui {class DialogConectarUDP;}

class DialogConectarUDP : public QDialog
{
        Q_OBJECT

    public:
        explicit DialogConectarUDP(QWidget *parent = nullptr);
        ~DialogConectarUDP();

        void setUdpSocket(bool *isConectUdp, QHostAddress *address, quint16 *port);

    private:
        Ui::DialogConectarUDP *ui;

        bool *isConectUdp;
        QHostAddress *address;
        quint16 *port;

        void closeEvent(QCloseEvent *)override;

    private slots:
        void on_pushButtonCancelar_clicked();
        void on_pushButtonConectar_clicked();
};

#endif // DIALOGCONECTARUDP_H
