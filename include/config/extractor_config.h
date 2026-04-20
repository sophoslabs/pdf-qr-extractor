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
#include <cstdint>
#include <thread>
#include <algorithm>

namespace extractor {

static constexpr uint32_t DEFAULT_QR_IMAGE_SIZE = 512;  // 500 KB
static constexpr uint32_t MAX_QR_IMAGE_SIZE = 1024;  // 1 MB

static constexpr uint32_t DEFAULT_PDF_SIZE_QR  = 512 ;  // 500 KB
static constexpr uint32_t MAX_PDF_SIZE_QR   = 1024; // 1 MB
static constexpr uint32_t DEFAULT_MAX_PDF_PAGES_FOR_QR_SCAN = 2;   // 2 PDF pages will be scanned by QR feature
static constexpr uint32_t MAX_PDF_PAGES_FOR_QR_SCAN = 10;  //  // Max 10 PDF pages will be scanned by QR feature
static constexpr uint32_t MAX_PAGE_WIDTH = 3000;  // 3000 pt (≈ 1 m at 72 dpi)
static constexpr uint32_t MAX_PAGE_HEIGHT = 3000;  // 3000 pt (≈ 1 m at 72 dpi)

static constexpr uint32_t DEFAULT_TIMEOUT_MS    = 2000; // 2000 ms

static constexpr uint32_t DEFAULT_LOGSIZE = 10 * 1024 * 1024; // 10 MB
static constexpr uint32_t MIN_LOGSIZE = 1 * 1024 * 1024; // 1 MB
static constexpr uint32_t MAX_LOGSIZE = 100 * 1024 * 1024; // 100 MB
static constexpr uint32_t DEFAULT_LOG_MAX_FILES = 5; // 5 files

static inline const std::string DEFAULT_LOG_LEVEL = "ERROR"; // Default error level

static inline uint32_t getDefaultWorkerThreads()
{
    unsigned int cores = std::thread::hardware_concurrency();

    if (cores == 0) {
        return 2; // safe fallback
    }

    // Use ~80% of cores, minimum 1
    return std::max(1u, static_cast<uint32_t>(cores * 0.8));
}

class ExtractorConfig {
public:
    std::string m_socketPath;
    
    uint32_t m_maxPagesToScan;
    uint32_t m_maxPdfSizeBytes;
    uint32_t m_maxQrImageBytes;
    

    uint32_t m_workerThreads;
    uint32_t m_requestTimeoutMs;   

    uid_t    m_allowedUid;

    uint32_t m_logSize;
    uint32_t m_maxLogFiles;
    bool     m_consoleOutput;
    std::string m_logFile;
    std::string m_logLevel;    

    void validate() const;  

};

ExtractorConfig loadExtractorConfig(const std::string& path);

}
