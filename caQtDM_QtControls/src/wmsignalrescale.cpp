#include "wmsignalrescale.h"
#include "qevent.h"
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

void wmSignalRescale::setRectSignalPosition(const QPoint &rectSignalPosition) {
    this->thisRectSignalPosition = rectSignalPosition;
}

QPoint wmSignalRescale::rectSignalPosition() const {
    return this->thisRectSignalPosition;
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
        if (this->parentWidget() != Q_NULLPTR && this->parentWidget()->objectName().length() > 0) {
            p.drawText(rect(), Qt::AlignCenter, this->parentWidget()->objectName() + " rescale");
        } else p.drawText(rect(), Qt::AlignCenter, "wmSignalRescale");
    }
}

bool wmSignalRescale::eventFilter(QObject *target, QEvent *event) {
    if (target == parent() && event->type() == QEvent::Resize) {
        QResizeEvent *resizeEvent = static_cast<QResizeEvent*>(event);
        QSize size = resizeEvent->size();
        emit internalResizeEvent(target, this, resizeEvent, thisSoftChannelA, thisSoftChannelB);

        emit emitSignal(QRect(thisRectSignalPosition, size));
        emit emitSignal(size);
        emit emitSignal(size.width(), size.height());
        emit emitSignalWidth(size.width());
        emit emitSignalHeight(size.height());
    }
    return QWidget::eventFilter(target, event);
}
