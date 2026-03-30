#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <algorithm>

namespace extractor {

enum class LogLevel {
    DEBUG = 0,
    INFO,
    WARN,
    ERROR,
    FATAL
};

inline LogLevel parseLogLevel(std::string level)
    {
        std::transform(level.begin(), level.end(), level.begin(), ::toupper);

        if (level == "DEBUG")
            return LogLevel::DEBUG;
        if (level == "INFO")
            return LogLevel::INFO;
        if (level == "WARN")
            return LogLevel::WARN;
        if (level == "ERROR")
            return LogLevel::ERROR;
        if (level == "FATAL")
            return LogLevel::FATAL;

        return LogLevel::INFO;
    }

class Logger {
public:
    static Logger& instance();

    void init(LogLevel level,
              bool consoleOutput,
              const std::string& filePath,
              uint64_t maxSizeBytes,
              uint32_t maxFiles);

    void log(LogLevel level,
             const char* file,
             int line,
             const std::string& component,
             const std::string& message);    

private:
    Logger() = default;
    ~Logger();

    void rotateIfNeeded();

    LogLevel m_level = LogLevel::INFO;
    bool m_consoleOutput = true;
    std::string m_filePath;
    uint64_t m_maxSizeBytes = 0;
    uint32_t m_maxFiles = 0;

    std::ofstream m_file;
    std::mutex m_mutex;

    bool isLogFileValid();
    void reopenIfNeeded();
};

} // namespace extractor

// ===========================
// Logging Macros
// ===========================

#define LOG_DEBUG(component, msg) \
    extractor::Logger::instance().log( \
        extractor::LogLevel::DEBUG, __FILE__, __LINE__, component, msg)

#define LOG_INFO(component, msg) \
    extractor::Logger::instance().log( \
        extractor::LogLevel::INFO, __FILE__, __LINE__, component, msg)

#define LOG_WARN(component, msg) \
    extractor::Logger::instance().log( \
        extractor::LogLevel::WARN, __FILE__, __LINE__, component, msg)

#define LOG_ERROR(component, msg) \
    extractor::Logger::instance().log( \
        extractor::LogLevel::ERROR, __FILE__, __LINE__, component, msg)

#define LOG_FATAL(component, msg) \
    extractor::Logger::instance().log( \
        extractor::LogLevel::FATAL, __FILE__, __LINE__, component, msg)
