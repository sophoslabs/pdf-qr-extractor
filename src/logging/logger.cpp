#include "logging/logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <filesystem>
#include <sys/stat.h>

namespace extractor
{

    Logger &Logger::instance()
    {
        static Logger inst;
        return inst;
    }

    void Logger::init(LogLevel level,
                      bool consoleOutput,
                      const std::string &filePath,
                      uint64_t maxSizeBytes,
                      uint32_t maxFiles)
    {
        m_level = level;
        m_consoleOutput = consoleOutput;
        m_filePath = filePath;
        m_maxSizeBytes = maxSizeBytes;
        m_maxFiles = maxFiles;

        if (!m_consoleOutput)
        {
            m_file.open(m_filePath, std::ios::app);
        }
    }

    Logger::~Logger()
    {
        if (m_file.is_open())
            m_file.close();
    }

    static std::string levelToString(LogLevel l)
    {
        switch (l)
        {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO ";
        case LogLevel::WARN:
            return "WARN ";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::FATAL:
            return "FATAL";
        }
        return "INFO";
    }

    void Logger::log(LogLevel level,
                 const char* file,
                 int line,
                 const std::string& component,
                 const std::string& message)
    {
        if (level < m_level)
            return;

        std::lock_guard<std::mutex> lock(m_mutex);

        // timestamp
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);

        std::tm tm{};
        localtime_r(&t, &tm);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
            << " | "
            << levelToString(level)
            << " | TID=" << std::this_thread::get_id()
            << " | "
            << component
            << " | "
            << file << ":" << line
            << " | "
            << message
            << "\n";

        if (m_consoleOutput)
        {
            std::cerr << oss.str();
        }
        else
        {
            reopenIfNeeded();

            if (m_file.is_open())
            {
                m_file << oss.str();
                m_file.flush();
                rotateIfNeeded();
            }
        }
    }

    bool Logger::isLogFileValid()
    {
        struct stat st{};
        return (stat(m_filePath.c_str(), &st) == 0);
    }

    void Logger::reopenIfNeeded()
    {
        if (!m_file.is_open() || !isLogFileValid())
        {
            if (m_file.is_open())
                m_file.close();

            m_file.open(m_filePath, std::ios::app);

            if (!m_file)
            {
                std::cerr << "[LOGGER] Failed to reopen log file: "
                          << m_filePath << std::endl;
            }
        }
    }

    void Logger::rotateIfNeeded()
    {
        if (m_maxSizeBytes == 0)
            return;

        if (std::filesystem::file_size(m_filePath) < m_maxSizeBytes)
            return;

        m_file.close();

        for (int i = m_maxFiles - 1; i >= 1; --i)
        {
            std::filesystem::rename(
                m_filePath + "." + std::to_string(i),
                m_filePath + "." + std::to_string(i + 1));
        }

        std::filesystem::rename(
            m_filePath,
            m_filePath + ".1");

        m_file.open(m_filePath, std::ios::trunc);
    }
}
