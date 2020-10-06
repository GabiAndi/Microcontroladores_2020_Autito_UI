#include "widgetpanel.h"

WidgetPanel::WidgetPanel(QWidget *parent) : QWidget(parent)
{

}

void WidgetPanel::paintEvent(QPaintEvent *)
{
    // Lienzo
    QPainter painter(this);

    // Variables
    int widgetMinSize = qMin(this->width(), this->height());
    float widgetScale = 1000.0;

    QFont font("MS Shell Dlg 2", 22);

    QPixmap autitoImage(QString("://src/graphics/Autito.png"));
    float autitoScale = 0.3;
    float autitoWidth = autitoImage.width() * autitoScale;
    float autitoHeight = autitoImage.height() * autitoScale;

    // Configuracion del lienzo
    painter.setRenderHint(QPainter::RenderHint::Antialiasing);

    painter.setFont(font);

    painter.translate(this->width() / 2.0, this->height() / 2.0);
    painter.scale(widgetMinSize / widgetScale,  widgetMinSize / widgetScale);

    // Dibujo del auto
    painter.drawPixmap(-autitoWidth / 2.0, -autitoHeight / 2.0,
                       autitoWidth, autitoHeight, autitoImage);

    // Dibujo de los valores de los sensores
    // Sensores laterales
    painter.drawText(-410, 110, "1023");
    painter.drawText(320, 110, "1023");

    // Sensores frontales laterales
    painter.drawText(-290, -60, "1023");
    painter.drawText(200, -60, "1023");

    // Sensores frontales cruzados
    painter.drawText(-130, -150, "1023");
    painter.drawText(60, -150, "1023");

    // Bateria
    font.setPointSize(38);
    painter.setFont(font);

    painter.drawText(-45, 360, "85\%");

    font.setPointSize(22);
}
