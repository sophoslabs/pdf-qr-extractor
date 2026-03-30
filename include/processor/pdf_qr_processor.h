#pragma once

#include "config/extractor_config.h"
#include <string>
#include <vector>

namespace extractor {

static constexpr int MAX_PAGE_SINGLE_PDF = 10;

/**
 * PDFQRProcessor
 *
 * Owns all PDF → QR logic.
 * No third-party types exposed in header.
 */
class PDFQRProcessor {
public:
    explicit PDFQRProcessor(const ExtractorConfig& cfg);

    /**
     * Extract ALL QR texts from PDF.
     * Result is newline-delimited.
     */
    bool extract(const std::string& pdfData, std::string& outCombinedResult, uint64_t rid);

private:
    const ExtractorConfig& m_cfg;

    std::vector<std::string>
    decodeQrFromImage(const std::vector<unsigned char>& pngData);
};

} // namespace extractor
