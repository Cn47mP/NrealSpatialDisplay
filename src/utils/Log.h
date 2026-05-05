#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <cstdio>
#include <cstdarg>
#include <windows.h>

enum class LogLevel { Debug, Info, Warn, Error };

namespace Log {

extern std::mutex g_logMutex;
extern std::ofstream g_logFile;
extern LogLevel g_minLevel;
extern LogLevel g_consoleLevel;
extern std::string g_logDir;

void Init(const std::string& dir = "logs", int keepDays = 2);
void Shutdown();
void Write(LogLevel level, const char* fmt, ...);

#define LOG_DEBUG(fmt, ...) Log::Write(LogLevel::Debug, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  Log::Write(LogLevel::Info,  fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  Log::Write(LogLevel::Warn,  fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) Log::Write(LogLevel::Error, fmt, ##__VA_ARGS__)

}
