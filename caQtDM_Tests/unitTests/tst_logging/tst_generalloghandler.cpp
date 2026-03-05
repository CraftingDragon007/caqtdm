#include "tst_generalloghandler.h"

#include "consoleloghandler.h"
#include "fileloghandler.h"
#include "generalloghandler.h"
#include "logstashloghandler.h"
#ifdef Q_OS_UNIX
#include "syslogloghandler.h"
#endif

#include <QTest>

#define ENV_LOG_LEVEL "CAQTDM_LOGGING_LEVEL"
#define ENV_LOG_HANDLERS "CAQTDM_LOGGING_HANDLERS"

static void mockMessageHandler(QtMsgType, const QMessageLogContext &, const QString &) {}

class MockLogHandler : public AbstractLogHandler
{
public:
    int handleLogCalls = 0;
    int flushCalls = 0;

    void handleLog(const Log &log) override
    {
        Q_UNUSED(log);
        handleLogCalls++;
    }

    void flush() override { flushCalls++; }
};

void TestGeneralLogHandler::initTestCase()
{
    // code to be executed before the first test function
}

void TestGeneralLogHandler::init()
{
    // code to be executed before each test function

    qunsetenv(ENV_LOG_LEVEL);
    qunsetenv(ENV_LOG_HANDLERS);
}

void TestGeneralLogHandler::cleanupTestCase()
{
    // code to be executed after the last test function
}

void TestGeneralLogHandler::cleanup()
{
    // code to be executed after each test function

    // Reverse everything done in GeneralLogHandler::initialize()
    qInstallMessageHandler(nullptr);

    QMutexLocker locker(&GeneralLogHandler::s_mutex);
    for (auto existingLogHandler : GeneralLogHandler::s_logHandlers) {
        delete existingLogHandler;
    }
    GeneralLogHandler::s_logHandlers.clear();

    if (GeneralLogHandler::s_logHandlersThread) {
        GeneralLogHandler::s_logHandlersThread->quit();
        GeneralLogHandler::s_logHandlersThread->wait();
        delete GeneralLogHandler::s_logHandlersThread;
        GeneralLogHandler::s_logHandlersThread = Q_NULLPTR;
    }
}

void TestGeneralLogHandler::injectsMessageHandlerAndReturnsPrevious()
{
    qInstallMessageHandler(mockMessageHandler);

    // This should return the previously injected handler
    QtMessageHandler previousHandler = GeneralLogHandler::initialize();
    QVERIFY(previousHandler == mockMessageHandler);

    // The currently installed handler should be GeneralLogHandler::messageHandler
    QtMessageHandler currentHandler = qInstallMessageHandler(nullptr);
    QVERIFY(currentHandler == GeneralLogHandler::messageHandler);
}

void TestGeneralLogHandler::initializationIsIdempotent()
{
    GeneralLogHandler::initialize();

    // Inject a handler that would be reset in case of complete (faulty) re-initialization
    auto *handler = new MockLogHandler();
    {
        QMutexLocker locker(&GeneralLogHandler::s_mutex);
        GeneralLogHandler::s_logHandlers.append(handler);
    }

    // This should return the already injected message handler
    QtMessageHandler previousHandler = GeneralLogHandler::initialize();
    QVERIFY(previousHandler == GeneralLogHandler::messageHandler);

    // And since the rest should not have been re-initialized, a log should still be received by the mock handler
    QVERIFY(std::any_of(GeneralLogHandler::s_logHandlers.begin(),
                        GeneralLogHandler::s_logHandlers.end(),
                        [handler](AbstractLogHandler *h) {
                            return dynamic_cast<MockLogHandler *>(h) == handler;
                        }));

    int previousCount = handler->handleLogCalls;
    qInfo() << "test";
    int currentCount = handler->handleLogCalls;
    QVERIFY(previousCount != currentCount);
}

void TestGeneralLogHandler::callsHandlerWithMinLogLevel()
{
    auto *handler = new MockLogHandler();

    GeneralLogHandler::s_minLogLevel = QtWarningMsg;

    {
        QMutexLocker locker(&GeneralLogHandler::s_mutex);
        GeneralLogHandler::s_logHandlers.append(handler);
    }

    // Info message should not invoke handler
    GeneralLogHandler::messageHandler(QtInfoMsg, {}, "info");
    QCOMPARE(handler->handleLogCalls, 0);

    // Warning message should invoke handler
    GeneralLogHandler::messageHandler(QtWarningMsg, {}, "warning");
    QCOMPARE(handler->handleLogCalls, 1);

    // Critical message should invoke handler
    GeneralLogHandler::messageHandler(QtCriticalMsg, {}, "critical");
    QCOMPARE(handler->handleLogCalls, 2);
}

void TestGeneralLogHandler::fatalMessageFlushesHandler()
{
    auto *handler = new MockLogHandler();

    {
        QMutexLocker locker(&GeneralLogHandler::s_mutex);
        GeneralLogHandler::s_logHandlers.append(handler);
    }

    GeneralLogHandler::messageHandler(QtFatalMsg, {}, "fatal");

    QCOMPARE(handler->flushCalls, 1);
}

void TestGeneralLogHandler::logLevelIsLoadedFromEnv()
{
    // Unset variable
    QCOMPARE(GeneralLogHandler::logLevelFromEnv(QtFatalMsg), QtFatalMsg);

    qputenv(ENV_LOG_LEVEL, "");
    QCOMPARE(GeneralLogHandler::logLevelFromEnv(QtWarningMsg), QtWarningMsg);

    qputenv(ENV_LOG_LEVEL, "all");
    QCOMPARE(GeneralLogHandler::logLevelFromEnv(QtWarningMsg), QtDebugMsg);
    qputenv(ENV_LOG_LEVEL, "debug");
    QCOMPARE(GeneralLogHandler::logLevelFromEnv(QtWarningMsg), QtDebugMsg);
    qputenv(ENV_LOG_LEVEL, "Qtdebugmsg");
    QCOMPARE(GeneralLogHandler::logLevelFromEnv(QtWarningMsg), QtDebugMsg);

    qputenv(ENV_LOG_LEVEL, "info");
    QCOMPARE(GeneralLogHandler::logLevelFromEnv(QtWarningMsg), QtInfoMsg);
    qputenv(ENV_LOG_LEVEL, "qtInfomsg");
    QCOMPARE(GeneralLogHandler::logLevelFromEnv(QtWarningMsg), QtInfoMsg);

    qputenv(ENV_LOG_LEVEL, "warning");
    QCOMPARE(GeneralLogHandler::logLevelFromEnv(QtWarningMsg), QtWarningMsg);
    qputenv(ENV_LOG_LEVEL, "qtwarningMsg");
    QCOMPARE(GeneralLogHandler::logLevelFromEnv(QtDebugMsg), QtWarningMsg);

    qputenv(ENV_LOG_LEVEL, "critical");
    QCOMPARE(GeneralLogHandler::logLevelFromEnv(QtDebugMsg), QtCriticalMsg);
    qputenv(ENV_LOG_LEVEL, "QtCriticalMsg");
    QCOMPARE(GeneralLogHandler::logLevelFromEnv(QtDebugMsg), QtCriticalMsg);

    qputenv(ENV_LOG_LEVEL, "fatal");
    QCOMPARE(GeneralLogHandler::logLevelFromEnv(QtDebugMsg), QtFatalMsg);
    qputenv(ENV_LOG_LEVEL, "QTFATALMSG");
    QCOMPARE(GeneralLogHandler::logLevelFromEnv(QtDebugMsg), QtFatalMsg);

    qputenv(ENV_LOG_LEVEL, "garbage");
    QCOMPARE(GeneralLogHandler::logLevelFromEnv(QtInfoMsg), QtInfoMsg);
}

void TestGeneralLogHandler::logHandlersAreInitializedFromEnv()
{
    // Only console
    qputenv(ENV_LOG_HANDLERS, "console");
    GeneralLogHandler::initialize();
    QVERIFY(std::any_of(GeneralLogHandler::s_logHandlers.begin(),
                        GeneralLogHandler::s_logHandlers.end(),
                        [](AbstractLogHandler *h) { return dynamic_cast<ConsoleLogHandler *>(h); }));

    cleanup();
    init();

    // Only file
    qputenv(ENV_LOG_HANDLERS, "fileloghandler");
    GeneralLogHandler::initialize();
    QVERIFY(std::any_of(GeneralLogHandler::s_logHandlers.begin(),
                        GeneralLogHandler::s_logHandlers.end(),
                        [](AbstractLogHandler *h) { return dynamic_cast<FileLogHandler *>(h); }));

    cleanup();
    init();

    // Only logstash
    qputenv(ENV_LOG_HANDLERS, "logstash");
    GeneralLogHandler::initialize();
    QVERIFY(
        std::any_of(GeneralLogHandler::s_logHandlers.begin(),
                    GeneralLogHandler::s_logHandlers.end(),
                    [](AbstractLogHandler *h) { return dynamic_cast<LogstashLogHandler *>(h); }));

    cleanup();
    init();

    // Console + file + logstash
    qputenv(ENV_LOG_HANDLERS, "logstashloghandler, consoleloghandler,file");
    GeneralLogHandler::initialize();
    QVERIFY(std::any_of(GeneralLogHandler::s_logHandlers.begin(),
                        GeneralLogHandler::s_logHandlers.end(),
                        [](AbstractLogHandler *h) { return dynamic_cast<ConsoleLogHandler *>(h); }));
    QVERIFY(std::any_of(GeneralLogHandler::s_logHandlers.begin(),
                        GeneralLogHandler::s_logHandlers.end(),
                        [](AbstractLogHandler *h) { return dynamic_cast<FileLogHandler *>(h); }));
    QVERIFY(
        std::any_of(GeneralLogHandler::s_logHandlers.begin(),
                    GeneralLogHandler::s_logHandlers.end(),
                    [](AbstractLogHandler *h) { return dynamic_cast<LogstashLogHandler *>(h); }));

    // for unix also check syslog (here: in combination with file and extra ',')
#ifdef Q_OS_UNIX
    qputenv(ENV_LOG_HANDLERS, "file,syslog,");
    GeneralLogHandler::initialize();
    QVERIFY(std::any_of(GeneralLogHandler::s_logHandlers.begin(),
                        GeneralLogHandler::s_logHandlers.end(),
                        [](AbstractLogHandler *h) { return dynamic_cast<SyslogLogHandler *>(h); }));
#endif
}
