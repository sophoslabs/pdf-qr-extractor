// IMPORTANT:
// This protocol header must be kept identical between
// extractor and daemon projects.
// Any incompatible change must increment PROTOCOL_VERSION.

#pragma once
#include <cstdint>

namespace extractor::protocol {

constexpr uint32_t QR_MAGIC = 0x51525152;   // "QRQR"
constexpr uint32_t PROTOCOL_VERSION = 1;

enum class QrStatus : uint32_t {
    OK = 0,
    LIMIT_EXCEEDED,
    VERSION_UNSUPPORTED,
    INVALID_REQUEST,
    INTERNAL_ERROR
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
