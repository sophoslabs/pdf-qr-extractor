# QR Extractor IPC Protocol Specification

## 1. Overview

The QR Extractor IPC protocol is a binary, framed, versioned protocol used for
communication between the SASI and the QR Extractor service over
UNIX domain sockets.

The protocol is designed to be:
- Deterministic
- Version-safe
- Extensible
- Resistant to framing errors
- Suitable for high-throughput concurrent usage

Each message consists of a fixed-size header followed by a variable-size payload.

## 2. Byte Order

All multi-byte integer fields use **host byte order**.  
Both SASI and extractor run on the same system architecture.

## 3. Request Format

### Request Header

| Field       | Size |    Type   |          Description            |
|------------ |------|-----------|---------------------------------|
| magic       | 4 B  | uint32_t  | Must equal QR_MAGIC (0x51525152)|
| version     | 4 B  | uint32_t  | Protocol version                |
| pdf_size    | 4 B  | uint32_t  | Size of PDF payload in bytes    |
| rid         | 8 B  | uint64_t  | Randomized Id (rid) for logging |

## 4. Response Format

### Response Header

| Field     | Size | Type     | Description |
|-----------|------|----------|-------------|
| status    | 4 B  | uint32_t | QrStatus code |
| data_size | 4 B  | uint32_t | Size of result payload |


## 5. Status Codes

| Code |       Name          |          Meaning             |
|------|---------------------|------------------------------|
|  0   | OK                  | Successful extraction        |
|  1   | LIMIT_EXCEEDED      | Configured limits violated   |
|  2   | VERSION_UNSUPPORTED | Protocol version mismatch    |
|  3   | INVALID_REQUEST     | Corrupt or malformed request |
|  4   | INTERNAL_ERROR      | Extractor processing failure |

## 6. Protocol Versioning

- Protocol version is strictly enforced.
- Extractor accepts only requests matching its supported version.
- Unsupported versions are rejected with VERSION_UNSUPPORTED.
- Backward compatibility is guaranteed only within the same protocol version.
- Any incompatible change requires incrementing the protocol version.

## 7. Framing Rules

- Message boundaries are defined only by header length fields.
- No implicit framing is assumed.
- Partial reads/writes must be handled by the transport layer.

## 8. Validation Rules

Extractor must reject requests when:

- magic is incorrect
- version is unsupported
- pdf_size is zero

No processing occurs after validation failure.

## 9. Extensibility

Future protocol versions may:
- Extend headers
- Add optional fields
- Add new status codes

Older extractors will reject incompatible versions explicitly.

## 10. Design Intent

This protocol provides a stable contract between SASI daemon and extractor,
allowing independent evolution while preserving safety and predictability.
