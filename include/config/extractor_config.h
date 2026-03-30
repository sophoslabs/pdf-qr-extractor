#pragma once
#include <string>
#include <cstdint>
#include <thread>
#include <algorithm>

namespace extractor {

// -------- Defaults with size in bytes ----------
static constexpr uint32_t DEFAULT_QR_IMAGE_SIZE = 512;  // 500 KB
static constexpr uint32_t MAX_QR_IMAGE_SIZE = 5000;  // 5 MB

static constexpr uint32_t DEFAULT_PDF_SIZE_QR  = 512 ;  // 500 KB
static constexpr uint32_t MAX_PDF_SIZE_QR   = 1000; // 1 MB
static constexpr uint32_t DEFAULT_MAX_PDF_PAGES_FOR_QR_SCAN = 2;   // 2 PDF pages will be scanned by QR feature
static constexpr uint32_t MAX_PDF_PAGES_FOR_QR_SCAN = 10;  //  // Max 10 PDF pages will be scanned by QR feature
static constexpr uint32_t MAX_PAGE_WIDTH = 3000;  // 3 KB
static constexpr uint32_t MAX_PAGE_HEIGHT = 3000;  // 3 KB

static constexpr uint32_t DEFAULT_TIMEOUT_MS    = 5000;

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
    uint32_t m_maxPdfSizeKiloBytes;
    uint32_t m_maxQrImageKiloBytes;
    

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
