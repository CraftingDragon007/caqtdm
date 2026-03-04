#ifndef TST_CONSOLELOGHANDLER_H
#define TST_CONSOLELOGHANDLER_H

#include <sstream>

#include <QObject>

class TestConsoleLogHandler : public QObject
{
    Q_OBJECT
public:
    TestConsoleLogHandler() = default;

private:
    class CustomCoutBuffer : public std::stringbuf
    {
    public:
        int flushedCounter = 0;

    protected:
        int sync() override
        {
            flushedCounter++;
            return std::stringbuf::sync();
        }
    };

    CustomCoutBuffer *m_buffer;
    std::streambuf *m_originalCout;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void cleanup();
    void parametersInitializedFromEnv();
    void verboseOutputToggleWorks();
    void flushToggleWorks();
};

#endif // TST_CONSOLELOGHANDLER_H
