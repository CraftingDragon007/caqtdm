#include "hmiapplicationeventfilter.h"


HMIApplicationEventFilter::HMIApplicationEventFilter(QObject *parent)
    : QObject{parent}
{}

bool HMIApplicationEventFilter::eventFilter(QObject *obj, QEvent *event){
    emit eventOccurred(obj, event);

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        emit keyPressed(obj, keyEvent);
    } else if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        emit mouse(obj, mouseEvent);
    }
    return QObject::eventFilter(obj, event);
}
