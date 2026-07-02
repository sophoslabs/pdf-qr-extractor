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

#define ASIO_STANDALONE
#include <asio.hpp>

#include "utils/response_utils.h"
#include "utils/asio_io_utils.h"
#include "logging/logger.h"

#include <iostream>

namespace extractor::utils {

using namespace extractor::protocol;
using namespace extractor::asio_utils;
using asio::local::stream_protocol;

bool send_error(asio::io_context& io,
                stream_protocol::socket& socket,
                QrStatus status,
                uint32_t timeoutMs)
{
    ResponseHeader rh{
        status,
        0
    };

    if (!write_full(io, socket,
                    asio::buffer(&rh, sizeof(rh)),
                    timeoutMs)) {
        LOG_WARN("IO","[WARN] Failed to send error response");
        return false;
    }

    return true;
}

} // namespace extractor::utils
