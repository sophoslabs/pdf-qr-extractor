// Copyright (C) 2026 Sophos Limited
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This file is part of pdf-qr-extractor.
//
// pdf-qr-extractor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// pdf-qr-extractor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with pdf-qr-extractor. If not, see <https://www.gnu.org/licenses/>

#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <algorithm>
#include <cstring>

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

    if (level == "DEBUG") return LogLevel::DEBUG;
    if (level == "INFO")  return LogLevel::INFO;
    if (level == "WARN")  return LogLevel::WARN;
    if (level == "ERROR") return LogLevel::ERROR;
    if (level == "FATAL") return LogLevel::FATAL;

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

//  Filename extraction macro

#ifndef __FILENAME__
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

// Logging Macros

#define LOG_DEBUG(component, msg) \
    extractor::Logger::instance().log( \
        extractor::LogLevel::DEBUG, __FILENAME__, __LINE__, component, msg)

#define LOG_INFO(component, msg) \
    extractor::Logger::instance().log( \
        extractor::LogLevel::INFO, __FILENAME__, __LINE__, component, msg)

#define LOG_WARN(component, msg) \
    extractor::Logger::instance().log( \
        extractor::LogLevel::WARN, __FILENAME__, __LINE__, component, msg)

#define LOG_ERROR(component, msg) \
    extractor::Logger::instance().log( \
        extractor::LogLevel::ERROR, __FILENAME__, __LINE__, component, msg)

#define LOG_FATAL(component, msg) \
    extractor::Logger::instance().log( \
        extractor::LogLevel::FATAL, __FILENAME__, __LINE__, component, msg)
