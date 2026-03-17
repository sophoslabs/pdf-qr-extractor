#include "utils/response_utils.h"
#include "utils/io_utils.h"

#include <unistd.h>
#include <iostream>

namespace extractor::utils {

using namespace extractor::protocol;

bool send_error(int fd,
                QrStatus status,
                uint32_t timeoutMs)
{
    ResponseHeader rh{
        status,
        0
    };

    if (!write_full(fd, &rh, sizeof(rh), timeoutMs)) {
        std::cerr << "[WARN] Failed to send error response\n";
        return false;
    }

    return true;
}

}
