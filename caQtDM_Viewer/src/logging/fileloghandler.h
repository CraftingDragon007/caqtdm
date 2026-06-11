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
 *  Copyright (c) 2010 - 2026
 *
 *  Author:
 *    Erik Schwarz
 *  Contact details:
 *    erik.schwarz@psi.ch
 */

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
Q_DECLARE_LOGGING_CATEGORY(fileLogHandlerLog)

class FileLogHandler : public QObject, public AbstractLogHandler
{
    Q_OBJECT
public:
    explicit FileLogHandler(QObject *parent = Q_NULLPTR);
    ~FileLogHandler() override;

    /**
     * @brief Buffers the specified log into a queue, to be asynchronously written.
     * This function is thread-safe.
     * @param log: The log to buffer
     */
    void handleLog(const Log &log) override;

public slots:
    /**
     * @brief Writes all currently buffered logs to the file.
     * This function is thread-safe.
     */
    void flush() override;

#ifndef UNIT_TESTING
private slots:
#endif
    /**
     * @brief Writes all currently buffered logs to the file.
     * This function is thread-safe but should not be accessed from other threads than the object's thread.
     */
    void clearLogBuffer();

#ifdef UNIT_TESTING
public:
#else
private:
#endif
    qint64 intFromEnv(const char *envName, const qint64 defaultValue);
    int fileCountFromEnv(const int defaultFileCount = DEFAULT_FILE_COUNT);
    /**
     * @brief Reads the log file max size from an env stored in bytes.
     * @param defaultFileSizeB: The default size in bytes
     * @return The final value to be used
     */
    qint64 fileSizeBFromEnv(const qint64 defaultFileSizeB = DEFAULT_FILE_SIZE_B);
    /**
     * @brief Reads the buffer timeout from an env stored in seconds, and returns it in milliseconds.
     * @param defaultTimeoutS: The default timeeout in seconds
     * @return The final value to be used
     */
    int bufferTimeoutMsFromEnv(const int defaultTimeoutS = DEFAULT_BUFFER_TIMEOUT_S);
    int bufferSizeFromEnv(const int defaultBufferSize = DEFAULT_BUFFER_SIZE);
    /**
     * @brief Deletes old files from logDir until there are only maxFiles left.
     * This deletes ALL kinds of files from logDir, so make sure logDir only contains log files.
     * @param logDir: The directory to clean
     * @param maxFiles: Maximum number of files to keep
     */
    void cleanupOldLogs(QDir logDir, const int maxFiles);
    /**
     * @brief Halves logFile, keeping the last half.
     * @param logFile: The log file to halve.
     */
    void halveLogFile(QFile &logFile);

    QString m_logFilePath;
    QList<Log> m_logBuffer;
    QMutex m_logFileMutex;
    QMutex m_logBufferMutex;
    QTimer *m_logBufferTimer;
    int m_logBufferTimeoutMs;
    int m_logBufferMaxSize;
    qint64 m_logFileMaxSizeB;
};

#endif // FILELOGHANDLER_H
