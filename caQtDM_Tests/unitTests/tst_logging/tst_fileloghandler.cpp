#include "tst_fileloghandler.h"

#include "fileloghandler.h"

#include <QStandardPaths>
#include <QTest>

#define ENV_FILE_COUNT "CAQTDM_LOGGING_FILE_COUNT"
#define ENV_FILE_SIZE "CAQTDM_LOGGING_FILE_SIZE"
#define ENV_BUFFER_TIMEOUT "CAQTDM_LOGGING_FILE_BUFFER_TIMEOUT"
#define ENV_BUFFER_SIZE "CAQTDM_LOGGING_FILE_BUFFER_SIZE"

// Note about logfile creation: These do NOT overwrite the regular caQtDM application logs.
// Instead, since this is a separate executable, they are put into the AppLocalDataLocation for tst_logging.

void TestFileLogHandler::initTestCase()
{
    // code to be executed before the first test function

    // Create log directory if necessary
    const QString logDirPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                               + "/Logs";
    QDir logDir(logDirPath);
    if (!logDir.exists()) {
        logDir.mkpath(logDirPath);
    }

    // And make sure its empty
    QFileInfoList files = logDir.entryInfoList(QDir::Files);
    for (const QFileInfo &fileInfo : files) {
        QFile::remove(fileInfo.absoluteFilePath());
    }
}

void TestFileLogHandler::init()
{
    // code to be executed before each test function

    qunsetenv(ENV_FILE_COUNT);
    qunsetenv(ENV_FILE_SIZE);
    qunsetenv(ENV_BUFFER_TIMEOUT);
    qunsetenv(ENV_BUFFER_SIZE);
}

void TestFileLogHandler::cleanupTestCase()
{
    // code to be executed after the last test function

    // Clean log directory again
    const QString logDirPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                               + "/Logs";
    QDir logDir(logDirPath);
    QFileInfoList files = logDir.entryInfoList(QDir::Files);
    for (const QFileInfo &fileInfo : files) {
        QFile::remove(fileInfo.absoluteFilePath());
    }
}

void TestFileLogHandler::cleanup()
{
    // code to be executed after each test function
}

void TestFileLogHandler::parametersInitializedFromEnv()
{
    qputenv(ENV_FILE_COUNT, "5");
    qputenv(ENV_FILE_SIZE, "1000");
    qputenv(ENV_BUFFER_TIMEOUT, "2");
    qputenv(ENV_BUFFER_SIZE, "3");

    FileLogHandler handler;

    QCOMPARE(handler.fileCountFromEnv(10), 5);
    QCOMPARE(handler.fileSizeBFromEnv(5000), 1000);
    QCOMPARE(handler.m_logFileMaxSizeB, 1000);
    QCOMPARE(handler.bufferTimeoutMsFromEnv(500), 2000); // env is in seconds
    QCOMPARE(handler.m_logBufferTimeoutMs, 2000);
    QCOMPARE(handler.bufferSizeFromEnv(10), 3);
    QCOMPARE(handler.m_logBufferMaxSize, 3);
}

void TestFileLogHandler::flushClearsBuffer()
{
    FileLogHandler handler;
    Log log;
    log.message = "log";

    handler.handleLog(log);
    QCOMPARE(handler.m_logBuffer.size(), 1);

    // Flush should synchronously empty the buffer
    handler.flush();
    QCOMPARE(handler.m_logBuffer.size(), 0);
}

void TestFileLogHandler::bufferMaxSizeFlushes()
{
    FileLogHandler handler;
    handler.m_logBufferMaxSize = 3;
    // If the timeout (10s) is reached during this test, it will break,
    // but then the performance is bad enough a fail is justified

    Log log1, log2, log3, log4;
    log1.message = "log1";
    log2.message = "log2";
    log3.message = "log3";
    log4.message = "log4";

    handler.handleLog(log1);
    handler.handleLog(log2);
    handler.handleLog(log3);
    QCOMPARE(handler.m_logBuffer.size(), 3);

    // Fourth log should cause a clear
    handler.handleLog(log4);
    // Wait for a second, it should not take longer than that to write async
    QEventLoop loop;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(1000);
    loop.exec();

    QCOMPARE(handler.m_logBuffer.size(), 0);
}

void TestFileLogHandler::bufferTimeoutFlushes()
{
    FileLogHandler handler;
    handler.m_logBufferTimeoutMs = 100;
    handler.m_logBufferTimer->setInterval(handler.m_logBufferTimeoutMs);

    Log log;
    log.message = "message";
    handler.handleLog(log);

    // Test signal being connected properly and flushing
    QEventLoop loop;
    QTimer singleShot;
    QObject::connect(&singleShot, &QTimer::timeout, &loop, &QEventLoop::quit);
    singleShot.start(1000); // Includes potential Signal & Slot delay
    loop.exec();

    QCOMPARE(handler.m_logBuffer.size(), 0);
}

void TestFileLogHandler::fileCreationAndTruncationWorks()
{
    FileLogHandler handler;
    const QString filePath = handler.m_logFilePath;

    QVERIFY(QFile::exists(filePath));

    // Small max file size to force truncation
    handler.m_logFileMaxSizeB = 150;

    Log log;
    log.message = "Some message thats not too short";

    // Write multiple times to exceed size
    for (int i = 0; i < 5; ++i) {
        handler.handleLog(log);
    }

    // The file will be around 225 bytes big, should be halved and should fall below the 150 bytes limit.
    handler.clearLogBuffer();

    QFile file(filePath);
    QVERIFY(file.exists());
    QVERIFY(file.size() <= handler.m_logFileMaxSizeB);
}

void TestFileLogHandler::cleanupOldLogsWorks()
{
    const QString logDirPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                               + "/Logs";
    QDir logDir(logDirPath);

    // Create 5 dummy files, there may be more already present, doesn't matter
    for (int i = 0; i < 5; ++i) {
        QFile file(logDir.filePath(QString("file_%1.log").arg(i)));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("test");
        file.close();
    }

    FileLogHandler handler;
    handler.cleanupOldLogs(logDir, 3);

    QFileInfoList remainingFiles = logDir.entryInfoList(QDir::Files);
    QCOMPARE(remainingFiles.size(), 3);
}
