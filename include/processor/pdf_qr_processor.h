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

#include "config/extractor_config.h"
#include "core/result.h"
#include <string>
#include <vector>
#include <cstdint>

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
     *
     * Returns structured result:
     *  - status: outcome classification
     *  - data: newline-delimited QR results (if any)
     */
    ExtractResult extract(const std::string& pdfData,
                      uint64_t rid,
                      std::chrono::steady_clock::time_point deadline);

private:
    const ExtractorConfig& m_cfg;

    std::vector<std::string>
    decodeQrFromImage(const std::vector<unsigned char>& pngData);
};

} // namespace extractor
