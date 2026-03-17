#pragma once
#include <string>
#include <cstdint>

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

static constexpr uint32_t DEFAULT_WORKERS       = 4;
static constexpr uint32_t DEFAULT_TIMEOUT_MS    = 5000;

static constexpr uint32_t DEFAULT_LOGSIZE = 10 * 1024 * 1024; // 10 MB
static constexpr uint32_t MIN_LOGSIZE = 1 * 1024 * 1024; // 1 MB
static constexpr uint32_t MAX_LOGSIZE = 100 * 1024 * 1024; // 100 MB

class ExtractorConfig {
public:
    std::string m_socketPath;
    
    uint32_t m_maxPagesToScan;
    uint32_t m_maxPdfSizeBytes;
    uint32_t m_maxQrImageBytes;
    

    uint32_t m_workerThreads;
    uint32_t m_requestTimeoutMs;   

    uid_t    m_allowedUid;

    void validate() const;
};

ExtractorConfig loadExtractorConfig(const std::string& path);

}
