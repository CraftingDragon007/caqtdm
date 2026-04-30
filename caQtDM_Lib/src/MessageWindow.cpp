/*
 *  This file is part of the caQtDM Framework, developed at the Paul Scherrer Institut,
 *  Villigen, Switzerland
 *
 *  The caQtDM Framework is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  The caQtDM Framework is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the caQtDM Framework.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Copyright (c) 2010 - 2014
 *
 *  Author:
 *    Anton Mezger
 *  Contact details:
 *    anton.mezger@psi.ch
 */


#include "MessageWindow.h"
#include "messageWindowWrapper.h"
#include "qapplication.h"
#include "qdatetime.h"
#include <QCoreApplication>
#include <QMutexLocker>
#include <stdio.h>
#include <time.h>
#include <QFile>
#include <QDebug>
#include <QTextStream>
#include <QScrollBar>
#ifndef MOBILE_ANDROID
#include <sys/timeb.h>
#else
#include <androidtimeb.h>
#endif
#include "qtdefinitions.h"

#define GCC_VERSION (__GNUC__ * 10000 \
                               + __GNUC_MINOR__ * 100 \
                               + __GNUC_PATCHLEVEL__)

const char* MessageWindow::WINDOW_TITLE = "caQtDM Messages";
MessageWindow* MessageWindow::MsgHandler = Q_NULLPTR;

Q_LOGGING_CATEGORY(messageWindowLog, "caqtdm.lib.messagewindow")
Q_LOGGING_CATEGORY(externCLog, "caqtdm.extern.c")

MessageWindow::MessageWindow(QWidget* parent) : QDockWidget(parent)
{
    m_logMessageEvents = !qEnvironmentVariableIsEmpty("CAQTDM_LOGGING_INCLUDE_MESSAGEWINDOW");

    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    msgTextEdit.setFont(font);

    QApplication* guiApp = qobject_cast<QApplication*>(qApp);
    QPalette palette = guiApp->palette();
    m_normalTextColorHex = palette.color(QPalette::Active, QPalette::Text).name();
    m_debugTextColorHex = palette.color(QPalette::Active, QPalette::Link).name();

    setFeatures(QDockWidget::NoDockWidgetFeatures);
    setWindowTitle(tr(WINDOW_TITLE));
    msgTextEdit.setReadOnly(true);
    msgTextEdit.document()->setMaximumBlockCount(400);
    setWidget(&msgTextEdit);
    MessageWindow::MsgHandler = this;
    setMinimumSize(600, 150);
    setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowMinMaxButtonsHint);
    setContextMenuPolicy(Qt::CustomContextMenu);
    show();

    move(x(), 0);
}

QString MessageWindow::QtMsgToQString(QtMsgType type, const char *msg)
{
    time_t          time_val;
    struct tm       *timess;
    struct timeb    timeA;
    char            prTime[200];

    ftime(&timeA);
    time_val = timeA.time;
    timess = localtime(&time_val);
    if(timess != Q_NULLPTR) {
        sprintf(prTime, "%02d-%02d-%04d %02d:%02d:%02d ", timess->tm_mday, timess->tm_mon+1, timess->tm_year+1900,  timess->tm_hour, timess->tm_min, timess->tm_sec);
        switch (type) {
                case QtDebugMsg:
                        return QString(prTime) + QString(msg);
                case QtWarningMsg:
                        return QString(prTime) + QString(msg);
                case QtCriticalMsg:
                        return QString(prTime) + QString(msg);
                case QtFatalMsg:
                        return QString(prTime) + QString(msg);
                default:
                        return QString(prTime) + QString(msg);
        }
     } else {
        return QString(msg);
     }
}

void MessageWindow::AppendMsgWrapper(QtMsgType type, char* msg)
{
        static QMutex mutex;
        QMutexLocker locker(&mutex);

        if (MessageWindow::MsgHandler != Q_NULLPTR)
                return MessageWindow::MsgHandler->postMsgEvent(type, msg);
        else
                fprintf(stderr, "%s\n", qasc(MessageWindow::QtMsgToQString(type, msg)));
}

void MessageWindow::customEvent(QEvent* event)
{
        if (static_cast<MessageWindow::EventType>(event->type()) == MessageWindow::MessageEvent) {
#ifdef __MINGW32__
                msgTextEdit.append(dynamic_cast<typename MessageEvent::MessageEvent* >(event)->msg);
#else
        #if defined(_WIN32)  || defined(__clang__)
                msgTextEdit.append(dynamic_cast<::MessageEvent* >(event)->msg);
        #else
               #if GCC_VERSION > 40407
                   msgTextEdit.append(dynamic_cast<typename MessageEvent::MessageEvent* >(event)->msg);
               #else
                   msgTextEdit.append(dynamic_cast<MessageEvent::MessageEvent* >(event)->msg);
               #endif
        #endif
#endif
        }
}

void MessageWindow::clearText()
{
    msgTextEdit.setPlainText("");
}

QString MessageWindow::getMessageBoxContents() {
    return msgTextEdit.toPlainText();
}

void MessageWindow::themeChanged() {
    QApplication* guiApp = qobject_cast<QApplication*>(qApp);
    QPalette palette = guiApp->palette();
    QString oldColorNormalHex = m_normalTextColorHex;
    QString oldColorDebugHex = m_debugTextColorHex;
    m_normalTextColorHex = palette.color(QPalette::Active, QPalette::Text).name();
    m_debugTextColorHex = palette.color(QPalette::Active, QPalette::Link).name();

    if (oldColorNormalHex != m_normalTextColorHex || oldColorDebugHex != m_debugTextColorHex) {
        redrawText(oldColorNormalHex, oldColorDebugHex);
    }
}

void MessageWindow::redrawText(const QString& oldNormalTextColorHex, const QString& oldDebugTextColorHex) {
    QString text = msgTextEdit.toHtml();
    text = text.replace(oldNormalTextColorHex, m_normalTextColorHex).replace(oldDebugTextColorHex, m_debugTextColorHex);
    msgTextEdit.setHtml(text);
    QScrollBar *vScrollBar = msgTextEdit.verticalScrollBar();
    if (vScrollBar) {
        vScrollBar->setValue(vScrollBar->maximum());
    }
}

void MessageWindow::postMsgEvent(QtMsgType type, char* msg)
{
    QString qmsg = MessageWindow::QtMsgToQString(type, msg);

    if (m_logMessageEvents) {
        // In addition to displaying the message in the message window, trigger a QtLogging message
        qt_message_output(type, QMessageLogContext("", 0, "", messageWindowLog().categoryName()), msg);
    }

    switch (type) {
#if QT_VERSION > QT_VERSION_CHECK(5, 0, 0)
    case QtInfoMsg:
        qmsg.prepend(QString("<FONT color=\"%1\">").arg(m_normalTextColorHex));
        qmsg.append("</FONT>");
        break;
#endif
    case QtDebugMsg:
        qmsg.prepend(QString("<FONT color=\"%1\">").arg(m_debugTextColorHex));
        qmsg.append("</FONT>");
        break;
    case QtWarningMsg:
        qmsg.prepend("<FONT color=\"#FF8C00\">");
        qmsg.append("</FONT>");
        break;
    case QtCriticalMsg:
    case QtFatalMsg:
        qmsg.prepend("<B><FONT color=\"#FF0000\">");
        qmsg.append("</FONT></B>");
        break;
    default:
        qmsg.prepend(QString("<FONT color=\"%1\">").arg(m_normalTextColorHex));
        qmsg.append("</FONT>");
        break;
    }
    //it's impossible to change GUI directly from thread other than the main thread
    //so post message encapsulated by MessageEvent to the main thread's event queue
#ifdef __MINGW32__
    QCoreApplication::postEvent(this, new typename MessageEvent::MessageEvent(qmsg));
#else
#if defined(_WIN32)  || defined(__clang__)
    QCoreApplication::postEvent(this, new ::MessageEvent(qmsg));
#else
#if GCC_VERSION > 40407
    QCoreApplication::postEvent(this, new typename MessageEvent::MessageEvent(qmsg));
#else
    QCoreApplication::postEvent(this, new MessageEvent::MessageEvent(qmsg));
#endif
#endif
#endif
}

extern "C" MessageWindow* C_postMsgEvent(MessageWindow* p, int type, char* msg)
{
    QtMsgType msgType;

    // Map QtMsgType
    switch (type) {
    case 0:
        msgType = QtDebugMsg;
        break;
    case 1:
        msgType = QtWarningMsg;
        break;
    case 2:
    case 3:
        msgType = QtCriticalMsg;
        break;
    default:
        return p;
        break;
    }

    // Trigger a QtLogging message since C cannot call QtLogging macros itself
    qt_message_output(msgType, QMessageLogContext("", 0, "", externCLog().categoryName()), msg);

    if(p == 0) return p;

    p->postMsgEvent(msgType, msg);
    return p;
}

void MessageWindow::closeEvent(QCloseEvent* ce)
{
    Q_UNUSED(ce);
}

MessageEvent::MessageEvent(QString & msg):
        QEvent(static_cast<QEvent::Type>(MessageWindow::MessageEvent))
{
        this->msg = msg;
}

