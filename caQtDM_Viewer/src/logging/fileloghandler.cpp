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
    QDir logDirectory = QDir(localAppDataDirectory).filePath(QSL("Logs"));

    if (!logDirectory.exists()) {
        if (!logDirectory.mkpath(logDirectory.path())) {
            qCCritical(fileLogHandler)
                << QSL("Failed to create log direcotry:") << logDirectory.path();
            return;
        }
    }

    const qint64 timestamp = QDateTime::currentSecsSinceEpoch();
    QFile logFile(logDirectory.filePath(QString::number(timestamp) + QSL(".log")));
    if (!logFile.open(QIODevice::ReadWrite)) {
        qCCritical(fileLogHandler) << QSL("Failed to create log file:") << logFile.fileName();
        return;
    }
    m_logFilePath = logFile.fileName();

    // Cleanup old logs, keep max number of files including current
    const int maxFiles = fileCountFromEnv();
    cleanupOldLogs(logDirectory, maxFiles);

    m_logFileMaxSizeB = fileSizeBFromEnv();
    m_logBufferMaxSize = bufferSizeFromEnv();
    m_logBufferTimeoutMs = bufferTimeoutMsFromEnv();
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
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(
            this,
            [=]() {
                if (m_logBufferTimer) {
                    delete m_logBufferTimer;
                }
            },
            Qt::BlockingQueuedConnection);
    } // Otherwise, it will automatically be cleaned up by Qt due to being a child of this
}

qint64 FileLogHandler::intFromEnv(const char *envName, const qint64 defaultValue)
{
    const QString valueString = qgetenv(envName);
    if (valueString.isEmpty()) {
        return defaultValue;
    }

    bool ok;
    const qint64 parsedValue = valueString.toLongLong(&ok);
    if (!ok) {
        qCWarning(fileLogHandler)
            << envName << QSL("is set and has a value, but could not be parsed to an int");
        return defaultValue;
    }

    return parsedValue;
}

int FileLogHandler::fileCountFromEnv(const int defaultFileCount)
{
    qint64 fileCount = intFromEnv(ENV_FILE_COUNT, defaultFileCount);
    if (fileCount < 1) {
        qCWarning(fileLogHandler) << ENV_FILE_COUNT
                                  << QSL("specified an invalid file count, must be positive");
        return defaultFileCount;
    }

    if (fileCount > INT_MAX) {
        qCWarning(fileLogHandler) << ENV_FILE_COUNT
                                  << QSL("specified invalid file count, must be smaller than")
                                  << INT_MAX;
        return defaultFileCount;
    }

    return fileCount;
}

qint64 FileLogHandler::fileSizeBFromEnv(const qint64 defaultFileSizeB)
{
    quint64 fileSizeB = intFromEnv(ENV_FILE_SIZE, defaultFileSizeB);
    if (fileSizeB < 1) {
        qCWarning(fileLogHandler) << ENV_FILE_SIZE
                                  << QSL("specified invalid file size, must be positive");
        return defaultFileSizeB;
    }

    return fileSizeB;
}

int FileLogHandler::bufferTimeoutMsFromEnv(const int defaultTimeoutS)
{
    qint64 bufferTimeoutS = intFromEnv(ENV_BUFFER_TIMEOUT, defaultTimeoutS);
    if (bufferTimeoutS < 1) {
        qCWarning(fileLogHandler) << ENV_BUFFER_TIMEOUT
                                  << QSL("specified invalid buffer timeout, must be positive");
        return defaultTimeoutS;
    }

    if (bufferTimeoutS
        > INT_MAX
              / 1000) { // Will be multiplied by 1000 afterwards to get MS, but cannot multiply in here due to potential overflow -> divide limit instead
        qCWarning(fileLogHandler) << ENV_BUFFER_TIMEOUT
                                  << QSL("specified invalid buffer timeout, must be smaller than")
                                  << INT_MAX / 1000;
        return defaultTimeoutS;
    }

    return bufferTimeoutS * 1000;
}

int FileLogHandler::bufferSizeFromEnv(const int defaultBufferSize)
{
    qint64 bufferSize = intFromEnv(ENV_BUFFER_SIZE, defaultBufferSize);
    if (bufferSize < 0) {
        qCWarning(fileLogHandler) << ENV_BUFFER_SIZE
                                  << QSL("specified invalid buffer size, must be >= 0");
        return defaultBufferSize;
    }

    if (bufferSize > INT_MAX) {
        qCWarning(fileLogHandler) << ENV_BUFFER_SIZE
                                  << QSL("specified invalid buffer size, must be smaller than")
                                  << INT_MAX;
        return defaultBufferSize;
    }

    return bufferSize;
}

void FileLogHandler::cleanupOldLogs(QDir logDir, const int maxFiles)
{
    logDir.setFilter(QDir::Files);
    logDir.setSorting(QDir::Time | QDir::Reversed); // Oldest first

    QFileInfoList files = logDir.entryInfoList();
    // Remove until only maxFiles remain
    while (files.size() > maxFiles) {
        QFileInfo oldest = files.takeFirst();
        QFile::remove(oldest.absoluteFilePath());
    }
}

void FileLogHandler::handleLog(const Log &log)
{
    bool shouldClear;
    {
        QMutexLocker locker(&m_logBufferMutex);
        m_logBuffer.append(log);
        shouldClear = m_logBuffer.size() > m_logBufferMaxSize;
    }

    if (shouldClear) {
        QMetaObject::invokeMethod(this, "clearLogBuffer", Qt::QueuedConnection);
    }
}

void FileLogHandler::flush()
{
    clearLogBuffer();
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
        logString.append(QSL("[") + log.timestampUtc + QSL("] ") + log.category + QSL(" | ")
                         + log.loglevelString + QSL(" | ") + log.locationString + QSL("> ")
                         + log.message + QSL("\n"));
    }

    QMutexLocker locker(&m_logFileMutex);

    QFile logFile(m_logFilePath);
    if (!logFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append)) {
        qCCritical(fileLogHandler) << QSL("Failed to open log file: ") << logFile.fileName();
        return;
    }

    if (logFile.write(qUtf8Printable(logString)) == -1) {
        qCCritical(fileLogHandler) << QSL("Failed to write log file: ") << logFile.fileName();
    }
    logFile.flush();

    if (logFile.size() > m_logFileMaxSizeB) {
        halveLogFile(logFile);
    }
    logFile.close();
}

void FileLogHandler::halveLogFile(QFile &logFile)
{
    const qint64 fileSize = logFile.size();
    const qint64 halfSize = fileSize / 2;

    // Read last half
    if (!logFile.seek(halfSize)) {
        qCCritical(fileLogHandler) << QSL("Failed to seek in log file");
        return;
    }
    const QByteArray lastHalf = logFile.read(fileSize - halfSize);

    // Truncate and write back last half
    logFile.resize(0);
    if (logFile.write(lastHalf) != lastHalf.size()) {
        qCCritical(fileLogHandler) << QSL("Failed to write truncated log file");
    }

    logFile.flush();
}
