#ifndef FILELOGHANDLER_H
#define FILELOGHANDLER_H

#include "abstractloghandler.h"

#include <QDir>
#include <QMutex>
#include <QTimer>

#define DEFAULT_FILE_COUNT 10
#define DEFAULT_FILE_SIZE_B 1000000
#define DEFAULT_BUFFER_TIMEOUT_S 10
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

#ifdef UNIT_TESTING
public:
#else
private:
#endif
    int intFromEnv(const char *envName, const int defaultValue);
    int fileCountFromEnv(const int defaultFileCount = DEFAULT_FILE_COUNT);
    int fileSizeBFromEnv(const int defaultFileSizeB = DEFAULT_FILE_SIZE_B);
    int bufferTimeoutMsFromEnv(const int defaultTimeoutS = DEFAULT_BUFFER_TIMEOUT_S);
    int bufferSizeFromEnv(const int defaultBufferSize = DEFAULT_BUFFER_SIZE);
    void cleanupOldLogs(QDir logDir, const int maxFiles);
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
