#include "fileloghandler.h"

#include <QDateTime>
#include <QDebug>
#include <QStandardPaths>
#include <QThread>

#define ENV_FILE_COUNT "CAQTDM_LOGGING_FILE_COUNT"
#define ENV_FILE_SIZE "CAQTDM_LOGGING_FILE_SIZE"
#define ENV_BUFFER_TIMEOUT "CAQTDM_LOGGING_FILE_BUFFER_TIMEOUT"
#define ENV_BUFFER_SIZE "CAQTDM_LOGGING_FILE_BUFFER_SIZE"

Q_LOGGING_CATEGORY(fileLogHandler, "logging.file");

FileLogHandler::FileLogHandler(QObject *parent)
    : QObject(parent)
{
    const QString localAppDataDirectory = QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation);
    const QDir logDirectory = QDir(localAppDataDirectory).filePath("Logs");

    if (!logDirectory.exists()) {
        logDirectory.mkpath(logDirectory.path());
    }

    qint64 timestamp = QDateTime::currentSecsSinceEpoch();
    QFile logFile(logDirectory.filePath(QString::number(timestamp) + ".log"));
    if (!logFile.open(QIODevice::ReadWrite)) {
        qCCritical(fileLogHandler) << "Failed to create log file:" << logFile.fileName();
        return;
    }
    m_logFilePath = logFile.fileName();

    // Cleanup old logs, keep max number of files including current
    int maxFiles = fileCountFromEnv();
    cleanupOldLogs(logDirectory, maxFiles);

    m_logFileMaxSizeB = fileSizeBFromEnv();
    m_logBufferTimeoutMs = bufferTimeoutMsFromEnv();
    m_logBufferMaxSize = bufferSizeFromEnv();
    m_logBufferTimer = new QTimer(this);
    QObject::connect(m_logBufferTimer,
                     &QTimer::timeout,
                     this,
                     &FileLogHandler::clearLogBuffer,
                     Qt::QueuedConnection);
    m_logBufferTimer->setInterval(m_logBufferTimeoutMs);
    m_logBufferTimer->start();
}

FileLogHandler::~FileLogHandler()
{
    QMutexLocker locker(&m_logFileMutex);
}

int FileLogHandler::fileCountFromEnv(int defaultFileCount)
{
    const QString fileCountString = qgetenv(ENV_FILE_COUNT);
    if (fileCountString.isEmpty()) {
        return defaultFileCount;
    }

    bool ok;
    const int fileCount = fileCountString.toInt(&ok);
    if (!ok) {
        qCWarning(fileLogHandler) << ENV_FILE_COUNT
                                  << "is set and has a value, but could not be parsed to an int";
        return defaultFileCount;
    }

    return fileCount;
}

int FileLogHandler::fileSizeBFromEnv(int defaultFileSizeB)
{
    const QString fileSizeString = qgetenv(ENV_FILE_SIZE);
    if (fileSizeString.isEmpty()) {
        return defaultFileSizeB;
    }

    bool ok;
    const int fileSizeB = fileSizeString.toInt(&ok); // Must be in bytes
    if (!ok) {
        qCWarning(fileLogHandler) << ENV_FILE_SIZE
                                  << "is set and has a value, but could not be parsed to an int";
        return defaultFileSizeB;
    }

    return fileSizeB;
}

int FileLogHandler::bufferTimeoutMsFromEnv(int defaultTimeoutMs)
{
    const QString timeoutString = qgetenv(ENV_BUFFER_TIMEOUT);
    if (timeoutString.isEmpty()) {
        return defaultTimeoutMs;
    }

    bool ok;
    const int timeout = timeoutString.toInt(&ok); // Must be in seconds (not ms!)
    if (!ok) {
        qCWarning(fileLogHandler) << ENV_BUFFER_TIMEOUT
                                  << "is set and has a value, but could not be parsed to an int";
        return defaultTimeoutMs;
    }

    return timeout * 1000;
}

int FileLogHandler::bufferSizeFromEnv(int defaultBufferSize)
{
    const QString bufferSizeString = qgetenv(ENV_BUFFER_SIZE);
    if (bufferSizeString.isEmpty()) {
        return defaultBufferSize;
    }

    bool ok;
    const int bufferSize = bufferSizeString.toInt(&ok);
    if (!ok) {
        qCWarning(fileLogHandler) << ENV_BUFFER_SIZE
                                  << "is set and has a value, but could not be parsed to an int";
        return defaultBufferSize;
    }

    return bufferSize;
}

void FileLogHandler::cleanupOldLogs(const QDir &logDir, int maxFiles)
{
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

void FileLogHandler::flush()
{
    if (QThread::currentThread() == this->thread()) {
        clearLogBuffer();
    } else {
        QMetaObject::invokeMethod(this, "clearLogBuffer", Qt::BlockingQueuedConnection);
    }
}

void FileLogHandler::clearLogBuffer()
{
    QList<Log> logs;
    {
        QMutexLocker locker(&m_logBufferMutex);
        logs = m_logBuffer;
        m_logBuffer.clear();
    }

    if (logs.empty()) {
        return;
    }

    QString logString;
    for (const auto &log : logs) {
        logString.append("[" + log.timestampUtc + "] " + log.category + " | " + log.loglevelString
                         + " | " + log.locationString + "> " + log.message + "\n");
    }

    QMutexLocker locker(&m_logFileMutex);

    QFile logFile(m_logFilePath);
    if (!logFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append)) {
        qCCritical(fileLogHandler) << "Failed to open log file: " << logFile.fileName();
        return;
    }

    logFile.write(qUtf8Printable(logString));
    logFile.flush();

    if (logFile.size() > m_logFileMaxSizeB) {
        truncateLogFile(logFile);
    }
    logFile.close();
}

void FileLogHandler::truncateLogFile(QFile &logFile)
{
    qint64 fileSize = logFile.size();
    qint64 halfSize = fileSize / 2;

    // Read last half
    if (!logFile.seek(halfSize)) {
        qCCritical(fileLogHandler) << "Failed to seek in log file";
        return;
    }
    QByteArray lastHalf = logFile.read(fileSize - halfSize);

    // Truncate and write back last half
    logFile.resize(0);
    if (logFile.write(lastHalf) != lastHalf.size()) {
        qCCritical(fileLogHandler) << "Failed to write truncated log file";
    }

    logFile.flush();
}
