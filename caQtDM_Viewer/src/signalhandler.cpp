#include "signalhandler.h"
#include <QCoreApplication>
#include <QDebug>

#ifdef _MSC_VER
#include <windows.h>
#else
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#endif

#ifndef _MSC_VER
int SignalHandler::signalSocketPair[2];

void SignalHandler::posixSignalHandler(int) {
    char a = 1;
    ::write(signalSocketPair[0], &a, sizeof(a));
}
#endif

#ifdef _MSC_VER
BOOL WINAPI windowsCtrlHandler(DWORD ctrlType) {
    if (ctrlType == CTRL_C_EVENT || ctrlType == CTRL_CLOSE_EVENT) {
        qDebug() << "Windows signal received. Exiting...";
        QCoreApplication::exit(0);
        return TRUE;
    }
    return FALSE;
}
#endif

SignalHandler::SignalHandler(QObject *parent) : QObject(parent) {
#ifndef _MSC_VER
    socketNotifier = new QSocketNotifier(signalSocketPair[1], QSocketNotifier::Read, this);
    connect(socketNotifier, &QSocketNotifier::activated, this, &SignalHandler::handleUnixSignal);
#endif
}

int SignalHandler::setupHandlers() {
#ifdef _MSC_VER
    return SetConsoleCtrlHandler(windowsCtrlHandler, TRUE) ? 0 : 1;
#else
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, signalSocketPair)) return 1;

    struct sigaction sa;
    sa.sa_handler = SignalHandler::posixSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (::sigaction(SIGINT, &sa, nullptr) != 0) return 2;
    if (::sigaction(SIGTERM, &sa, nullptr) != 0) return 3;
    return 0;
#endif
}

#ifndef _MSC_VER
void SignalHandler::handleUnixSignal() {
    socketNotifier->setEnabled(false);
    char tmp;
    ::read(signalSocketPair[1], &tmp, sizeof(tmp));

    qDebug() << "Unix signal received. Exiting...";
    QCoreApplication::exit(0);
}
#endif
