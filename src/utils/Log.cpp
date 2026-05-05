#include "Log.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace Log {
    std::mutex g_logMutex;
    std::ofstream g_logFile;
    LogLevel g_minLevel = LogLevel::Debug;
    LogLevel g_consoleLevel = LogLevel::Info;
    std::string g_logDir = "logs";

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
            auto now = fs::file_time_type::clock::now();
            for (const auto& entry : fs::directory_iterator(dir)) {
                if (!entry.is_regular_file()) continue;
                std::string filename = entry.path().filename().string();
                if (filename.find("nreal_") != 0 || filename.find(".log") == std::string::npos) continue;
                auto age = now - entry.last_write_time();
                if (age > std::chrono::hours(24 * keepDays)) {
                    DeleteFileA(entry.path().string().c_str());
                }
            }
        } catch (...) {}
    }

    void Init(const std::string& dir, int keepDays) {
        g_logDir = dir;

        DWORD attr = GetFileAttributesA(dir.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) {
            CreateDirectoryA(dir.c_str(), nullptr);
        }

        CleanupOldLogs(dir, keepDays);

        std::string path = dir + "/nreal_" + GetDateStr() + ".log";
        g_logFile.open(path, std::ios::out | std::ios::app);
    }

    void Shutdown() {
        if (g_logFile.is_open()) g_logFile.close();
    }

    void Write(LogLevel level, const char* fmt, ...) {
        if (level < g_minLevel) return;

        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        std::lock_guard<std::mutex> lock(g_logMutex);
        std::string line = "[" + GetTimestamp() + "] [" + LevelStr(level) + "] " + buf + "\n";

        if (level >= g_consoleLevel) {
            OutputDebugStringA(line.c_str());
            printf("%s", line.c_str());
        }

        if (g_logFile.is_open()) {
            g_logFile.write(line.c_str(), line.size());
            g_logFile.flush();
        }
    }
}
