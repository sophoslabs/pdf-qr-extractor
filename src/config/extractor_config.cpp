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

#include "config/extractor_config.h"
#include "config/config.h"
#include "logging/logger.h"
#include <cstdint>
#include <iostream>
#include <pwd.h>
#include <thread>

namespace extractor {

using Config = extractor::util::Config;

//  Helpers

static uint32_t readUIntWithDefault(Config& c,
                                    const std::string& key,
                                    uint32_t DEFAULT,
                                    uint32_t min,
                                    uint32_t max)
{
    std::string sizeUnit;
    if(key.find("KB") != std::string::npos) { sizeUnit = "KB"; }
    try {        
        int v = c.getInt(key);

        if (v < static_cast<int>(min)) {
            std::cerr << "[WARN] " << key
                      << " out of range (" << v
                      << "). Using default " << DEFAULT << " " << sizeUnit << "\n";
            return DEFAULT;
        }

        if (v > static_cast<int>(max)) {
            std::cerr << "[WARN] " << key
                      << " out of range (" << v
                      << "). Using max " << max << " " << sizeUnit << "\n";
            return max;
        }        
        return static_cast<uint32_t>(v);
    }
    catch (const std::exception& e) {
        std::cerr << "[WARN] " << key
                  << " missing or invalid. Using default "
                  << DEFAULT << " " << sizeUnit <<"\n";
        return DEFAULT;
    }
}

static bool readBoolWithDefault(Config& c,
                                const std::string& key,
                                bool DEFAULT)
{
    try
    {
        auto v = c.getBool(key);       
        return v;
    }
    catch (const std::exception& e) {
        std::cerr << "[WARN] " << key
                  << " missing or invalid. Using default "
                  << DEFAULT << "\n";
        return DEFAULT;
    }
}

uint32_t readLogSize(extractor::util::Config& raw)
{
    try {
        unsigned long v = raw.getULong("LOG_SIZE");

        if (v < MIN_LOGSIZE) {
            std::cerr << "[WARN] LOG_SIZE below minimum "
                      << MIN_LOGSIZE << ", using minimum\n";
            return MIN_LOGSIZE;
        }

        if (v > MAX_LOGSIZE) {
            std::cerr << "[WARN] LOG_SIZE above maximum "
                      << MAX_LOGSIZE << ", using maximum\n";
            return MAX_LOGSIZE;
        }        
        return static_cast<uint32_t>(v);
    }
    catch (const std::exception& e) {
        std::cerr << "[WARN] LOG_SIZE invalid or missing: "
                  << e.what()
                  << ", using default " << DEFAULT_LOGSIZE << "\n";
        return DEFAULT_LOGSIZE;
    }
}

static uid_t readRequiredUid(Config& c, const std::string& key)
{
    int v = c.getInt(key);

    if (v < 0)
        throw std::runtime_error("ALLOWED_UID must be >= 0");

    struct passwd* pw = getpwuid(static_cast<uid_t>(v));
    if (!pw)
        throw std::runtime_error("ALLOWED_UID does not map to a valid user");    
    return static_cast<uid_t>(v);
}

//  Main Loader 

ExtractorConfig loadExtractorConfig(const std::string& path)
{
    Config raw(path);

    ExtractorConfig cfg{};

    uint32_t defaultWorkerThreads = getDefaultWorkerThreads();
    uint32_t cores = std::thread::hardware_concurrency();

    try {
        cfg.m_socketPath = raw.getString("EXTRACTOR_SOCKET_PATH");
    }
    catch (...) {
        throw std::runtime_error("EXTRACTOR_SOCKET_PATH is mandatory");
    }
    
    cfg.m_maxPagesToScan = readUIntWithDefault(raw, "MAX_PDF_PAGES_FOR_QR_SCAN", DEFAULT_MAX_PDF_PAGES_FOR_QR_SCAN, 1, MAX_PDF_PAGES_FOR_QR_SCAN);

    cfg.m_maxPdfSizeBytes = readUIntWithDefault(raw, "MAX_PDF_QR_DECODE_KB_SIZE", DEFAULT_PDF_SIZE_QR, 1, MAX_PDF_SIZE_QR) * 1024;

    cfg.m_maxQrImageBytes = readUIntWithDefault(raw, "MAX_QR_DECODE_KB_SIZE", DEFAULT_QR_IMAGE_SIZE, 1, MAX_QR_IMAGE_SIZE) * 1024;

    cfg.m_workerThreads = readUIntWithDefault(raw, "WORKER_THREADS", defaultWorkerThreads, 1, cores);
    if(cfg.m_workerThreads > cores - 1)
    {
        LOG_ERROR("CONFIG", "Configured WORKER_THREADS= " + std::to_string(cfg.m_workerThreads) + " is close to CPU cores= "  
        + std::to_string(cores) + "This may cause contention and increased latency under load.");
    }    

    cfg.m_requestTimeoutMs = readUIntWithDefault(raw, "REQUEST_TIMEOUT_MS", DEFAULT_TIMEOUT_MS, 100, 3000);

    cfg.m_allowedUid =  readRequiredUid(raw, "ALLOWED_UID");

    cfg.m_consoleOutput = readBoolWithDefault(raw, "CONSOLE_OUTPUT", false);

    cfg.m_logSize = readLogSize(raw);

     cfg.m_maxLogFiles = readUIntWithDefault(raw, "MAX_LOG_FILES", DEFAULT_LOG_MAX_FILES, 1, 50);

    try {
    cfg.m_logFile = raw.getString("LOG_FILE");
    }
     catch (...) {
        cfg.m_logFile = "/tmp/extractor.log";
        LOG_ERROR ("CONFIG", "Log File not provided in configuration file, writting logs to default path :- /tmp/extractor.log");
    }

    try {
    cfg.m_logLevel = raw.getString("LOG_LEVEL");
    }
     catch (...) {
        cfg.m_logLevel = DEFAULT_LOG_LEVEL;
        LOG_ERROR ("CONFIG", "Log Level not provided in configuration file, assigning default, ERROR level");
    }

    cfg.validate();
    return cfg;
}

// Validation 

void ExtractorConfig::validate() const
{
    if (m_socketPath.empty())
        throw std::runtime_error("Internal error: Socket path empty");

    if (m_maxPagesToScan > MAX_PDF_PAGES_FOR_QR_SCAN)
        throw std::runtime_error("Internal error: MAX_PDF_PAGES_FOR_QR_SCAN exceeded");

    if (m_workerThreads == 0)
        throw std::runtime_error("Internal error: WORKER_THREADS invalid");

    if (m_maxQrImageBytes == 0)
        throw std::runtime_error("Internal error: Invalid QR image size");

    if (m_maxPdfSizeBytes == 0)
        throw std::runtime_error("Internal error: Invalid PDF size");

    if (m_logSize == 0)
        throw std::runtime_error("Internal error: Invalid log size");

    if(m_requestTimeoutMs == 0)
        throw std::runtime_error("Internal error: Invalid request timeout");
}

} // namespace extractor
