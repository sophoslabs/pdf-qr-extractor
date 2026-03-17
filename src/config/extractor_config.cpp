#include "config/extractor_config.h"
#include "config/config.h"
#include <cstdint>
#include <iostream>
#include <pwd.h>

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
        std::cerr << key << "= " << v << std::endl;
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
        std::cerr << key << "= " << v << std::endl;
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
        std::cerr << "LOG_SIZE = " << v << std::endl;
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
    std::cerr << key << "= " << v << std::endl;
    return static_cast<uid_t>(v);
}



//  Main Loader 

ExtractorConfig loadExtractorConfig(const std::string& path)
{
    Config raw(path);

    ExtractorConfig cfg{};

    try {
        cfg.m_socketPath = raw.getString("EXTRACTOR_SOCKET_PATH");
    }
    catch (...) {
        throw std::runtime_error("EXTRACTOR_SOCKET_PATH is mandatory");
    }

    cfg.m_maxPagesToScan =
        readUIntWithDefault(raw, "MAX_PDF_PAGES_FOR_QR_SCAN",
                            DEFAULT_MAX_PDF_PAGES_FOR_QR_SCAN, 1, MAX_PDF_PAGES_FOR_QR_SCAN);

    cfg.m_maxPdfSizeBytes =
        readUIntWithDefault(raw, "MAX_PDF_QR_DECODE_KB_SIZE",
                            DEFAULT_PDF_SIZE_QR, 1, MAX_PDF_SIZE_QR) * 1000;

    cfg.m_maxQrImageBytes =
        readUIntWithDefault(raw, "MAX_QR_DECODE_KB_SIZE",
                            DEFAULT_QR_IMAGE_SIZE, 1, MAX_QR_IMAGE_SIZE) * 1000;

    cfg.m_workerThreads =
        readUIntWithDefault(raw, "WORKER_THREADS",
                            DEFAULT_WORKERS, 1, 64);

    cfg.m_requestTimeoutMs =
        readUIntWithDefault(raw, "REQUEST_TIMEOUT_MS",
                            DEFAULT_TIMEOUT_MS, 100, 60000);

    cfg.m_allowedUid =
        readRequiredUid(raw, "ALLOWED_UID");

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

    if (m_requestTimeoutMs == 0)
        throw std::runtime_error("Internal error: DEFAULT_TIMEOUT_MS invalid");

    if (m_maxQrImageBytes == 0)
        throw std::runtime_error("Internal error: Invalid QR image size");
}

} // namespace extractor
