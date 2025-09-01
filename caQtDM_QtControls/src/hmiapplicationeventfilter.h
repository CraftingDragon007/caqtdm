#ifndef HMIAPPLICATIONEVENTFILTER_H
#define HMIAPPLICATIONEVENTFILTER_H

#include <QObject>
#include <qevent.h>

class HMIApplicationEventFilter : public QObject
{
    Q_OBJECT
public:
    explicit HMIApplicationEventFilter(QObject *parent = nullptr);

signals:
    void keyPressed(QObject *target, QKeyEvent *event);
    void mousePressed(QObject *target, QMouseEvent *event);
    void eventOccurred(QObject *target, QEvent *event);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // HMIAPPLICATIONEVENTFILTER_H
