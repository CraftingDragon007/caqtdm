#ifndef FILELOGHANDLER_H
#define FILELOGHANDLER_H

#include "abstractloghandler.h"

#include <QDir>
#include <QMutex>
#include <QTimer>

#define DEFAULT_FILE_COUNT 10
#define DEFAULT_FILE_SIZE_B 1000000
#define DEFAULT_BUFFER_TIMEOUT_MS 10000
#define DEFAULT_BUFFER_SIZE 20

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(fileLogHandler);

class FileLogHandler : public QObject, public AbstractLogHandler
{
    Q_OBJECT
public:
    explicit FileLogHandler(QObject *parent = Q_NULLPTR);
    ~FileLogHandler() override;

    void handleLog(const Log &log) override;

public slots:
    void flush() override;
    void clearLogBuffer();

private:
    int fileCountFromEnv(int defaultFileCount = DEFAULT_FILE_COUNT);
    int fileSizeBFromEnv(int defaultFileSizeB = DEFAULT_FILE_SIZE_B);
    int bufferTimeoutMsFromEnv(int defaultTimeoutMs = DEFAULT_BUFFER_TIMEOUT_MS);
    int bufferSizeFromEnv(int defaultBufferSize = DEFAULT_BUFFER_SIZE);
    void cleanupOldLogs(const QDir &logDir, int maxFiles);
    void truncateLogFile(QFile &logFile);

    QString m_logFilePath;
    QList<Log> m_logBuffer;
    QMutex m_logFileMutex;
    QMutex m_logBufferMutex;
    QTimer *m_logBufferTimer;
    int m_logBufferTimeoutMs;
    int m_logBufferMaxSize;
    int m_logFileMaxSizeB;
};

#endif // FILELOGHANDLER_H
