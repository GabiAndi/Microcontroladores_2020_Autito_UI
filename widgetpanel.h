#ifndef WIDGETPANEL_H
#define WIDGETPANEL_H

#include <QWidget>
#include <QPaintEvent>
#include <QPainter>
#include <QBrush>
#include <QPen>

class WidgetPanel : public QWidget
{
        Q_OBJECT
    public:
        explicit WidgetPanel(QWidget *parent = nullptr);

    private:
        void paintEvent(QPaintEvent *)override;

    signals:

};

#endif // WIDGETPANEL_H
