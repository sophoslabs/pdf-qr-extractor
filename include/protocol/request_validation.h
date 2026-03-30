#pragma once

#include "protocol/pdf_qr_protocol.h"
#include "config/extractor_config.h"

namespace extractor::protocol {

QrStatus validateRequest(const RequestHeader& r, const ExtractorConfig& cfg);

}
