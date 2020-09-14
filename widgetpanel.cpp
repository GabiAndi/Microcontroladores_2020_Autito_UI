#include "widgetpanel.h"

WidgetPanel::WidgetPanel(QWidget *parent) : QWidget(parent)
{

}

void WidgetPanel::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    //painter.drawEllipse(100, 100, 100, 100);
}
