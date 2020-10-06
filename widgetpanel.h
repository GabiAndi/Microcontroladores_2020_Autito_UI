#ifndef WIDGETPANEL_H
#define WIDGETPANEL_H

#include <QWidget>
#include <QPaintEvent>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QPixmap>
#include <QFont>

#include <stdint.h>

class WidgetPanel : public QWidget
{
        Q_OBJECT
    public:
        explicit WidgetPanel(QWidget *parent = nullptr);

        void setSensor1Value(uint16_t sensor1Value);
        void setSensor2Value(uint16_t sensor2Value);
        void setSensor3Value(uint16_t sensor3Value);
        void setSensor4Value(uint16_t sensor4Value);
        void setSensor5Value(uint16_t sensor5Value);
        void setSensor6Value(uint16_t sensor6Value);

    private:
        void paintEvent(QPaintEvent *)override;

        uint16_t sensor1Value = 0;
        uint16_t sensor2Value = 0;
        uint16_t sensor3Value = 0;
        uint16_t sensor4Value = 0;
        uint16_t sensor5Value = 0;
        uint16_t sensor6Value = 0;
};

#endif // WIDGETPANEL_H
