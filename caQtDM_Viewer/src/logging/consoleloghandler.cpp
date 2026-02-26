#include "consoleloghandler.h"

#include <iostream>

ConsoleLogHandler::ConsoleLogHandler() {}

ConsoleLogHandler::~ConsoleLogHandler() {}

void ConsoleLogHandler::handleLog(const Log &log) {
    std::cout << "Console Log: " << qUtf8Printable(log.message) << "\n";
}
