#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include <QObject>
#include <QSocketNotifier>

class SignalHandler : public QObject
{
    Q_OBJECT
public:
    explicit SignalHandler(QObject *parent = nullptr);
    static int setupHandlers();

private:
#ifndef _MSC_VER
    static int signalSocketPair[2];
    static void posixSignalHandler(int sig);
    QSocketNotifier *socketNotifier;
private slots:
    void handleUnixSignal();
#endif
};

#endif // SIGNALHANDLER_H
