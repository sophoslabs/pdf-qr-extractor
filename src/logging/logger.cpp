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
            mode_t oldMask = umask(0077);
            m_file.open(m_filePath, std::ios::app);
            umask(oldMask);
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
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        }
        return "INFO";
    }

    void Logger::log(LogLevel level,
                 const char* file,
                 int line,
                 const std::string& component,
                 const std::string& message)
    try
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
    catch (const std::exception& e)
    {
        std::cerr << "[LOGGER] log() failed: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "[LOGGER] log() failed: unknown exception" << std::endl;
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

        try {
            if (std::filesystem::file_size(m_filePath) < m_maxSizeBytes)
                return;
        }
        catch (const std::filesystem::filesystem_error&) {
            // File was deleted externally — reopen it and skip rotation
            m_file.open(m_filePath, std::ios::app);
            return;
        }

        m_file.close();

        for (int i = m_maxFiles - 1; i >= 1; --i)
        {
            std::string src = m_filePath + "." + std::to_string(i);
            std::string dst = m_filePath + "." + std::to_string(i + 1);
            if (std::filesystem::exists(src))
                std::filesystem::rename(src, dst);
        }

        if (std::filesystem::exists(m_filePath))
            std::filesystem::rename(m_filePath, m_filePath + ".1");

        m_file.open(m_filePath, std::ios::trunc);
    }
}
