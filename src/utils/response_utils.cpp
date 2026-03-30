#define ASIO_STANDALONE
#include <asio.hpp>

#include "utils/response_utils.h"
#include "utils/asio_io_utils.h"

#include <iostream>

namespace extractor::utils {

using namespace extractor::protocol;
using namespace extractor::asio_utils;
using asio::local::stream_protocol;

bool send_error(stream_protocol::socket& socket,
                QrStatus status,
                uint32_t timeoutMs)
{
    ResponseHeader rh{
        status,
        0
    };

    if (!write_full(socket,
                    asio::buffer(&rh, sizeof(rh)),
                    timeoutMs)) {
        std::cerr << "[WARN] Failed to send error response\n";
        return false;
    }

    return true;
}

} // namespace extractor::utils
