#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <cstdio>
#include <cstdarg>
#include <windows.h>
#include <filesystem>

enum class LogLevel { Debug, Info, Warn, Error };

namespace Log {

extern std::mutex g_logMutex;
extern std::ofstream g_logFile;
extern LogLevel g_minLevel;
extern LogLevel g_consoleLevel;
extern std::string g_logDir;

inline const char* LevelStr(LogLevel lv) {
    switch (lv) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO ";
        case LogLevel::Warn:  return "WARN ";
        case LogLevel::Error: return "ERROR";
    }
    return "?????";
}

inline std::string GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    struct tm tmBuf;
    localtime_s(&tmBuf, &t);
    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03d",
             tmBuf.tm_hour, tmBuf.tm_min, tmBuf.tm_sec, (int)ms.count());
    return buf;
}

inline std::string GetDateStr() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    struct tm tmBuf;
    localtime_s(&tmBuf, &t);
    char buf[16];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
             tmBuf.tm_year + 1900, tmBuf.tm_mon + 1, tmBuf.tm_mday);
    return buf;
}

inline void CleanupOldLogs(const std::string& dir, int keepDays) {
    try {
        auto now = std::chrono::system_clock::now();
        auto cutoff = now - std::chrono::hours(24 * keepDays);

        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;

            std::string filename = entry.path().filename().string();
            if (filename.find("nreal_") != 0 || filename.find(".log") == std::string::npos) continue;

            auto ftime = std::chrono::system_clock::from_time_t(
                std::chrono::duration_cast<std::chrono::seconds>(
                    entry.last_write_time().time_since_epoch()).count());

            if (ftime < cutoff) {
                DeleteFileA(entry.path().string().c_str());
                OutputDebugStringA(("Log: Deleted old log: " + entry.path().string() + "\n").c_str());
            }
        }
    } catch (...) {
    }
}

inline void Init(const std::string& dir = "logs", int keepDays = 2) {
    g_logDir = dir;

    DWORD attr = GetFileAttributesA(dir.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        BOOL created = CreateDirectoryA(dir.c_str(), nullptr);
        if (!created) {
            DWORD err = GetLastError();
            char errMsg[256];
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, 0, errMsg, sizeof(errMsg), nullptr);
            OutputDebugStringA(("Log: CreateDirectory failed: " + std::string(errMsg)).c_str());
        }
    }

    CleanupOldLogs(dir, keepDays);

    std::string path = dir + "\\nreal_" + GetDateStr() + ".log";
    g_logFile.open(path, std::ios::out | std::ios::app);

    if (!g_logFile.is_open()) {
       OutputDebugStringA(("Log: Failed to open " + path + "\n").c_str());
    } else {
       OutputDebugStringA(("Log: Opened " + path + "\n").c_str());
    }
}

inline void Shutdown() {
    if (g_logFile.is_open()) g_logFile.close();
}

inline void Write(LogLevel level, const char* fmt, ...) {
    if (level < g_minLevel) return;

    char msgBuf[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msgBuf, sizeof(msgBuf), fmt, args);
    va_end(args);

    std::string ts = GetTimestamp();
    std::string line = "[" + ts + "] [" + LevelStr(level) + "] " + msgBuf;

    std::lock_guard<std::mutex> lock(g_logMutex);

    if (level >= g_consoleLevel) {
        OutputDebugStringA((line + "\n").c_str());
    }

    if (g_logFile.is_open()) {
        g_logFile << line << "\n";
        g_logFile.flush();
    }
}

} // namespace Log

#define LOG_DEBUG(fmt, ...) Log::Write(LogLevel::Debug, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  Log::Write(LogLevel::Info,  fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  Log::Write(LogLevel::Warn,  fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) Log::Write(LogLevel::Error, fmt, ##__VA_ARGS__)