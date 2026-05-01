
#include "Log.h"

namespace Log {
    std::mutex g_logMutex;
    std::ofstream g_logFile;
    LogLevel g_minLevel = LogLevel::Debug;
    LogLevel g_consoleLevel = LogLevel::Info;
    std::string g_logDir = "logs";
}
