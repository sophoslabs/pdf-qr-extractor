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

#include "core/handlers.h"
#include "logging/logger.h"

namespace extractor
{

PdfHandler::PdfHandler(PDFQRProcessor &processor)
    : processor_(processor)
{
}

ExtractResult PdfHandler::handle(const IMessage &msg,
                                 uint64_t rid,
                                 std::chrono::steady_clock::time_point deadline)
{
    const std::string &pdfData = msg.raw();

    // PDF spec (ISO 32000-1 §7.5.2) allows %PDF within the first 1024 bytes
    static constexpr size_t kSearchWindow = 1024;
    static constexpr std::string_view kPdfMagic = "%PDF";

    auto searchLen = std::min(pdfData.size(), kSearchWindow);
    auto pos = pdfData.find(kPdfMagic, 0);

    if (pos == std::string::npos || pos >= searchLen) {
        LOG_WARN("HANDLER", "RID=" + std::to_string(rid) + " Rejected: missing PDF magic bytes");
        ExtractResult res;
        res.status = ExtractStatus::INVALID_INPUT;
        return res;
    }

    return processor_.extract(pdfData, rid, deadline);
}

ExtractResult DefaultHandler::handle(const IMessage &msg,
                                     uint64_t rid,
                                     std::chrono::steady_clock::time_point)
{
    LOG_WARN("HANDLER", "RID=" + std::to_string(rid) + " Unknown message type: " + msg.getType());
    ExtractResult res;
    res.status = ExtractStatus::INVALID_INPUT;
    return res;
}

} // namespace extractor
