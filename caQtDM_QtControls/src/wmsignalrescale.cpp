#include "wmsignalrescale.h"
#include <QApplication>
#include <QPainter>


wmSignalRescale::wmSignalRescale(QWidget *parent)
    : QWidget{parent}
{

}

void wmSignalRescale::setSoftChannelA(const QString &softChannelA) {
    this->thisSoftChannelA = softChannelA;
}

QString wmSignalRescale::softChannelA() const {
    return this->thisSoftChannelA;
}

void wmSignalRescale::setSoftChannelB(const QString &softChannelB) {
    this->thisSoftChannelB = softChannelB;
}

QString wmSignalRescale::softChannelB() const {
    return this->thisSoftChannelB;
}

bool wmSignalRescale::isDesignerMode() {
    return qApp->property("APP_SOURCE") == QString("DESIGNER");
}

void wmSignalRescale::paintEvent(QPaintEvent *ev) {
    QWidget::paintEvent(ev);
    if (this->isDesignerMode()) {
        QPainter p(this);
        p.setPen(Qt::black);
        p.fillRect(rect(), Qt::GlobalColor::darkYellow);
        p.drawText(rect(), Qt::AlignCenter, "wmSignalRescale");
    }
}
