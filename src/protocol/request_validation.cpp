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

#include "protocol/request_validation.h"
#include "logging/logger.h"

namespace extractor::protocol {

QrStatus validateRequestHeader(const RequestHeader& r, const ExtractorConfig& cfg) {
    if (r.magic != QR_MAGIC)
        return QrStatus::INVALID_REQUEST;

    if (r.version != PROTOCOL_VERSION) {
        LOG_ERROR("PROTOCOL", "PDF QR Extractor protocol version unsupported: "
                  "received=" + std::to_string(r.version) +
                  " expected=" + std::to_string(PROTOCOL_VERSION));
        return QrStatus::VERSION_UNSUPPORTED;  
    }

    if (r.pdf_size == 0 || r.pdf_size > cfg.m_maxPdfSizeBytes) {
        LOG_ERROR("PROTOCOL", "PDF size limit exceeded: "
                  "received=" + std::to_string(r.pdf_size) +
                  " limit=" + std::to_string(cfg.m_maxPdfSizeBytes));
        return QrStatus::LIMIT_EXCEEDED;
    }

    return QrStatus::OK;
}

}
