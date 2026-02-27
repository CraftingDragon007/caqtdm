#include "fileloghandler.h"

#include <iostream>

#include <QDateTime>
#include <QDebug>
#include <QStandardPaths>

FileLogHandler::FileLogHandler() : QObject(Q_NULLPTR) {
    const QString localAppDataDirectory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QDir logDirectory = QDir(localAppDataDirectory).filePath("Logs");

    if (!logDirectory.exists()) {
        logDirectory.mkpath(logDirectory.path());
    }

    qint64 timestamp = QDateTime::currentSecsSinceEpoch();
    QFile logFile(logDirectory.filePath(QString::number(timestamp) + ".log"));
    if (!logFile.open(QIODevice::ReadWrite)) {
        qCritical() << "Failed to create log file:" << logFile.fileName();
        return;
    }
    m_logFilePath = logFile.fileName();

    // Cleanup old logs, keep max 10 files including current
    cleanupOldLogs(logDirectory, 10);

    m_logFileMaxSizeB = 1000000; // 1MB
    m_logBufferTimeoutMs = 10000;
    m_logBufferMaxSize = 5;
    m_logBufferTimer = new QTimer(this);
    QObject::connect(m_logBufferTimer, &QTimer::timeout, this, &FileLogHandler::clearLogBuffer, Qt::QueuedConnection);
    m_logBufferTimer->setInterval(m_logBufferTimeoutMs);
    m_logBufferTimer->start();
}

FileLogHandler::~FileLogHandler() {
    QMutexLocker locker(&m_logFileMutex);
    QMetaObject::invokeMethod(m_logBufferTimer, &QTimer::stop, Qt::BlockingQueuedConnection);
}

void FileLogHandler::cleanupOldLogs(const QDir &logDir, int maxFiles) {
    QDir dir(logDir);
    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Time | QDir::Reversed); // Oldest first

    QFileInfoList files = dir.entryInfoList();
    // Remove until only maxFiles remain
    while (files.size() > maxFiles) {
        QFileInfo oldest = files.takeFirst();
        QFile::remove(oldest.absoluteFilePath());
    }
}

void FileLogHandler::handleLog(const Log &log)
{
    {
        QMutexLocker locker(&m_logBufferMutex);
        m_logBuffer.append(log);
    }

    if (m_logBuffer.size() > m_logBufferMaxSize) {
        QMetaObject::invokeMethod(this, "clearLogBuffer", Qt::QueuedConnection);
    }
}

void FileLogHandler::clearLogBuffer() {
    QList<Log> logs;
    {
        QMutexLocker locker(&m_logBufferMutex);
        // This is done because const-ness othewise breaks compilation
        logs = m_logBuffer;
        m_logBuffer.clear();
    }


    QString logString;
    for (const auto &log: logs) {
        logString.append("[" + log.timestampUtc + "] " + log.loglevelString + " | "
                         + log.locationString + "> " + log.message + "\n");
    }

    QMutexLocker locker(&m_logFileMutex);

    QFile logFile(m_logFilePath);
    if (!logFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append)) {
        qCritical() << "Failed to open log file: " << logFile.fileName();
        return;
    }

    qDebug() << "\"" << qUtf8Printable(logString) << "\"";
    logFile.write(qUtf8Printable(logString));
    logFile.flush();

    if (logFile.size() > m_logFileMaxSizeB) {
        truncateLogFile(logFile);
    }

    logFile.close();
}

void FileLogHandler::truncateLogFile(QFile &logFile) {
    qint64 fileSize = logFile.size();
    qint64 halfSize = fileSize / 2;

    // Read last half
    if (!logFile.seek(halfSize)) {
        qCritical() << "Failed to seek in log file";
        return;
    }
    QByteArray lastHalf = logFile.read(fileSize - halfSize);

    // Truncate and write back last half
    logFile.resize(0);
    if (logFile.write(lastHalf) != lastHalf.size()) {
        qWarning() << "Failed to write truncated log file";
    }

    logFile.flush();
}
