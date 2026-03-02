#ifndef FILELOGHANDLER_H
#define FILELOGHANDLER_H

#include "abstractloghandler.h"

#include <QDir>
#include <QMutex>
#include <QTimer>

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
