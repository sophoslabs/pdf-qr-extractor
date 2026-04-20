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

// IMPORTANT:
// This protocol header must be kept identical between
// extractor and client projects.
// Any incompatible change must increment PROTOCOL_VERSION.

#pragma once
#include <cstdint>

namespace extractor::protocol {

constexpr uint32_t QR_MAGIC = 0x51525152;   // "QRQR"
constexpr uint32_t PROTOCOL_VERSION = 1;

enum class QrStatus : uint32_t {
    OK                  = 0,
    NO_QR               = 1,
    LIMIT_EXCEEDED      = 2,
    VERSION_UNSUPPORTED = 3,
    INVALID_REQUEST     = 4,
    INTERNAL_ERROR      = 5,
    TIMEOUT             = 6,
    NOT_AVAILABLE       = 7
};

#pragma pack(push,1)

struct RequestHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t pdf_size;
    uint64_t rid;
};

struct ResponseHeader {
    QrStatus status;
    uint32_t data_size;
};

#pragma pack(pop)

}
