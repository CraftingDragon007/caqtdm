#include "signalhandler.h"
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <QCoreApplication>

int SignalHandler::sighandlerSockets[2];

SignalHandler::SignalHandler(QObject *parent) : QObject(parent)
{
    socketNotifier = new QSocketNotifier(sighandlerSockets[1], QSocketNotifier::Read, this);
    connect(socketNotifier, &QSocketNotifier::activated, this, &SignalHandler::handleSocketData);
}

int SignalHandler::setupHandlers()
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sighandlerSockets)) {
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = SignalHandler::posixSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (::sigaction(SIGINT, &sa, nullptr) != 0) return 2;
    if (::sigaction(SIGTERM, &sa, nullptr) != 0) return 3;

    return 0;
}

void SignalHandler::posixSignalHandler(int sig)
{
    ::write(sighandlerSockets[0], &sig, sizeof(sig));
}

void SignalHandler::handleSocketData()
{
    socketNotifier->setEnabled(false);
    int signalNumber;
    ::read(sighandlerSockets[1], &signalNumber, sizeof(signalNumber));

    if (signalNumber == SIGINT) {
        emit interruptReceived();
    } else if (signalNumber == SIGTERM) {
        emit terminateReceived();
    }

    socketNotifier->setEnabled(true);
}
