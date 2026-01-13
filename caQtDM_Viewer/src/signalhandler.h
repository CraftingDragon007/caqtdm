#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include <QObject>
#include <QSocketNotifier>

class SignalHandler : public QObject
{
    Q_OBJECT
public:
    explicit SignalHandler(QObject *parent = nullptr);

    static void posixSignalHandler(int sig);

    static int setupHandlers();

signals:
    void interruptReceived();
    void terminateReceived();

private slots:
    void handleSocketData();

private:
    static int sighandlerSockets[2];
    QSocketNotifier *socketNotifier;
};

#endif // SIGNALHANDLER_H
