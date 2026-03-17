#pragma once

#include <cstdint>
#include "protocol/pdf_qr_protocol.h"

namespace extractor::utils {

/**
 * Sends an error response (status only, no payload)
 */
bool send_error(int fd,
                extractor::protocol::QrStatus status,
                uint32_t timeoutMs);

}
