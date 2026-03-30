#include "protocol/request_validation.h"
#include <iostream>

namespace extractor::protocol {

QrStatus validateRequest(const RequestHeader& r, const ExtractorConfig& cfg) {
    if (r.magic != QR_MAGIC)
        return QrStatus::INVALID_REQUEST;

    if (r.version != PROTOCOL_VERSION) {
        std::cerr << "[ERROR] SASI protocol version unsupported, exiting.";
        return QrStatus::VERSION_UNSUPPORTED;  
    }

    if (r.pdf_size == 0 || r.pdf_size > cfg.m_maxPdfSizeKiloBytes) {
        std::cerr << "[ERROR] pdf size limit execeeded, exiting.";
        return QrStatus::LIMIT_EXCEEDED;
    }

    return QrStatus::OK;
}

}
