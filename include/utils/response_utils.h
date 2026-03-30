#pragma once

#define ASIO_STANDALONE
#include <asio.hpp>

#include "protocol/pdf_qr_protocol.h"

namespace extractor::utils {

using asio::local::stream_protocol;

bool send_error(asio::io_context& io,
                stream_protocol::socket& socket,
                extractor::protocol::QrStatus status,
                uint32_t timeoutMs);

}
