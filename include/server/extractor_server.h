#pragma once

#include "worker_pool.h"
#include "config/extractor_config.h"
#include "processor/pdf_qr_processor.h"

#include <string>

namespace extractor {

class ExtractorServer {
public:
    explicit ExtractorServer(const ExtractorConfig& cfg);
    ~ExtractorServer();

    // Main accept loop
    void run();

private:
    // Listening socket FD
    int m_listenFd;

    // Immutable runtime configuration
    ExtractorConfig m_cfg;

    // Real QR processor (poppler + stb + zxing)
    PDFQRProcessor m_qrProcessor;

    WorkerPool m_workerPool;

    // Socket lifecycle
    int createListenSocket();
    void cleanupSocket();

    // Handles exactly one client request synchronously
    void handleClient(int clientFd);
};

} // namespace extractor
