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
    painter.drawText(-410, 110, QString::asprintf("%u", sensor6Value));
    painter.drawText(320, 110, QString::asprintf("%u", sensor1Value));

    // Sensores frontales laterales
    painter.drawText(-290, -60, QString::asprintf("%u", sensor2Value));
    painter.drawText(200, -60, QString::asprintf("%u", sensor3Value));

    // Sensores frontales cruzados
    painter.drawText(-130, -150, QString::asprintf("%u", sensor4Value));
    painter.drawText(60, -150, QString::asprintf("%u", sensor5Value));
}

void WidgetPanel::setSensor1Value(uint16_t sensor1Value)
{
    this->sensor1Value = sensor1Value;

    update();
}

void WidgetPanel::setSensor2Value(uint16_t sensor2Value)
{
    this->sensor2Value = sensor2Value;

    update();
}

void WidgetPanel::setSensor3Value(uint16_t sensor3Value)
{
    this->sensor3Value = sensor3Value;

    update();
}

void WidgetPanel::setSensor4Value(uint16_t sensor4Value)
{
    this->sensor4Value = sensor4Value;

    update();
}

void WidgetPanel::setSensor5Value(uint16_t sensor5Value)
{
    this->sensor5Value = sensor5Value;

    update();
}

void WidgetPanel::setSensor6Value(uint16_t sensor6Value)
{
    this->sensor6Value = sensor6Value;

    update();
}

void WidgetPanel::setBateryValue(uint16_t bateryValue)
{
    this->bateryValue = bateryValue;

    update();
}
