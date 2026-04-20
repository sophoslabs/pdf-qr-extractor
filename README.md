# pdf-qr-extractor

Secure, high-throughput PDF QR code extraction service for Linux. Runs as a
non-root systemd daemon and exposes a binary IPC protocol over a Unix domain
socket, designed to be called by a client process in-process or from a separate
service.

## Overview

`pdf-qr-extractor` accepts a raw PDF over a Unix domain socket, renders each
page using **poppler**, decodes QR codes using **ZXing**, and returns the
decoded text to the caller. It is purpose-built for high-concurrency pipelines
where scanning thousands of email-attached PDFs is a normal workload.

Key properties:
- Binary, versioned, framed IPC protocol — no silent corruption on mismatch
- Worker pool with per-request deadlines — no unbounded blocking
- Peer UID verification via `SO_PEERCRED` — only authorised callers accepted
- Single-instance enforcement — prevents accidental duplicate deployments
- Structured log rotation with configurable level and size limits


## Architecture


Client (UDSClient)
    │  Unix domain socket (binary protocol)
    ▼
ExtractorServer        ← accepts connections, enforces security checks
    │
WorkerPool             ← thread pool, bounded queue, per-task deadline
    │
Dispatcher             ← routes by message type ("pdf")
    │
PdfHandler             ← PDF magic bytes check (ISO 32000-1 §7.5.2)
    │
PDFQRProcessor         ← poppler render → STB PNG → ZXing QR decode


## Dependencies

All third-party libraries are vendored under `third_party/`.

| Library      | Version  | Use                          |
|--------------|----------|------------------------------|
| poppler-cpp  | vendored | PDF rendering                |
| ZXing-cpp    | vendored | QR code decoding (static)    |
| ASIO         | vendored | Async socket I/O (standalone)|
| STB image    | vendored | PNG encode/decode (header)   |

Build requirements: **C++17**, **CMake 3.16+**, **pthreads**.

## Build

# Configure (Release)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j$(nproc)

# Binary output
./build/pdf_qr_extractor


For a debug build:

cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

## Configuration

Copy and edit `pdf_qr_extractor.conf` before installation.

| Key                      | Description                              | Default        |
|--------------------------|------------------------------------------|----------------|
| `EXTRACTOR_SOCKET_PATH`  | Unix domain socket path                  | (required)     |
| `ALLOWED_UID`            | UID of the authorised client process     | (required)     |
| `WORKER_THREADS`         | Number of processing threads             | ~80% of cores  |
| `REQUEST_TIMEOUT_MS`     | Per-request deadline in milliseconds     | 500            |
| `MAX_PDF_PAGES_FOR_QR_SCAN` | Pages scanned per PDF (max 10)        | 2              |
| `MAX_PDF_QR_DECODE_KB_SIZE` | Max PDF payload in KB (max 1024)      | 512            |
| `MAX_QR_DECODE_KB_SIZE`  | Max rendered image size in KB (max 5120) | 512            |
| `LOG_FILE`               | Path to log file                         | (required)     |
| `LOG_LEVEL`              | DEBUG / INFO / WARN / ERROR / FATAL      | ERROR          |
| `LOG_SIZE`               | Log rotation size in MB (1–100)          | 10             |

## Installation

Run from the directory containing the built binary, `pdf_qr_extractor.conf`,
and `scripts/pdf_qr_extractor.service`:

sudo bash scripts/install_pdf_qr_extractor.sh

This will:
1. Create a dedicated system user `pdfqr`
2. Install the binary to `/opt/pdf-qr-extractor/bin/`
3. Install config to `/opt/pdf-qr-extractor/config/`
4. Install and enable the systemd service

Check service status:

systemctl status pdf_qr_extractor
journalctl -u pdf_qr_extractor -f

Uninstall:

sudo bash scripts/uninstall_pdf_qr_extractor.sh

## IPC Protocol

The wire protocol is a compact binary format. See [docs/protocol.md](docs/protocol.md) for the full specification.

**Request:** `RequestHeader` (20 bytes) + raw PDF payload  
**Response:** `ResponseHeader` (8 bytes) + optional QR text payload

Magic: `0x51525152` ("QRQR") · Protocol version: `1`

Status codes: `OK`, `NO_QR`, `LIMIT_EXCEEDED`, `VERSION_UNSUPPORTED`,
`INVALID_REQUEST`, `INTERNAL_ERROR`, `TIMEOUT`, `NOT_AVAILABLE`

## Security Model

- Runs as non-root system user (`pdfqr`)
- Socket directory permissions restrict filesystem access
- Peer UID validated on every connection via `SO_PEERCRED`
- Socket path must be absolute, within a non-world-writable directory, and must not be a symlink
- Single-instance lock prevents concurrent daemon processes
- Log files created with mode `0600`
- systemd hardening: `PrivateDevices`, `ProtectSystem=strict`, `ProtectHome`

## Platform

Ubuntu 20.04 LTS and later · x86-64 Linux only

## License

See [LICENSE](LICENSE).

