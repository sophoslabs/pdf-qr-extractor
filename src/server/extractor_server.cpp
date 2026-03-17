#include "server/extractor_server.h"
#include "protocol/request_validation.h"
#include "utils/io_utils.h"
#include "system/shutdown.h"
#include "security/security_checks.h"

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <iostream>

using namespace extractor::protocol;
using namespace extractor::security;

namespace extractor {

ExtractorServer::ExtractorServer(const ExtractorConfig& cfg)
    : m_listenFd(-1),
      m_cfg(cfg),
      m_qrProcessor(cfg),
      m_workerPool(cfg.m_workerThreads)
{
    enforceNotRoot();
    validateSocketPath(m_cfg.m_socketPath);
    validateSocketDirectory(m_cfg.m_socketPath);
    enforceSingleInstance(m_cfg.m_socketPath);
    m_listenFd = createListenSocket();
}


ExtractorServer::~ExtractorServer()
{
    m_workerPool.shutdown();
    cleanupSocket();
}

int ExtractorServer::createListenSocket()
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
        throw std::runtime_error("socket() failed");

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, m_cfg.m_socketPath.c_str(),
            sizeof(addr.sun_path) - 1);

    unlink(m_cfg.m_socketPath.c_str());

    if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error(std::string("bind() failed: ") + strerror(errno));

    chmod(m_cfg.m_socketPath.c_str(), 0600);

    if (listen(fd, m_cfg.m_workerThreads * 2) < 0)
        throw std::runtime_error("listen() failed");

    return fd;
}

void ExtractorServer::cleanupSocket()
{
    if (m_listenFd >= 0) {
        close(m_listenFd);
        m_listenFd = -1;
    }
    unlink(m_cfg.m_socketPath.c_str());
}

void ExtractorServer::handleClient(int fd)
{
    using namespace extractor::protocol;

    try {

        // Verify connecting process' identity
        verifyPeerUid(fd, m_cfg.m_allowedUid);

    } catch (const std::exception& e) {
        std::cerr << "[SECURITY] Connection rejected: "
                  << e.what() << "\n";
        close(fd);
        return;
    }

    RequestHeader hdr{};
    if (!read_full(fd, &hdr, sizeof(hdr), m_cfg.m_requestTimeoutMs)) {
        std::cerr << "[WARN] Failed to read request header (timeout or disconnect)\n";
        close(fd);
        return;
    }
    std::cerr << "SERVER: Request header received fd= "<< fd << ", hdr= "<< hdr.pdf_size << " bytes" << std::endl;

    // Protocol validation (ONLY structure/version)
    QrStatus st = validateRequest(hdr, m_cfg);
    if (st != QrStatus::OK) {
        std::cerr << "[INFO] Invalid request received, status="
                  << static_cast<uint32_t>(st) << "\n";

        ResponseHeader rh{st, 0};
        write_full(fd, &rh, sizeof(rh), m_cfg.m_requestTimeoutMs);
        std::cerr << "SERVER: Sending response header fd= "<< fd << ", hdr= "<< rh.data_size << " bytes" << std::endl;
        close(fd);
        return;
    }

    // Policy / config validation (limits)
    if (hdr.pdf_size > m_cfg.m_maxPdfSizeBytes ||
        hdr.pdf_size == 0) {

        ResponseHeader rh{QrStatus::LIMIT_EXCEEDED, 0};
        write_full(fd, &rh, sizeof(rh), m_cfg.m_requestTimeoutMs);
        close(fd);
        return;
    }

    // Safe allocation AFTER validation
    std::string pdf;
    pdf.resize(hdr.pdf_size);

    if (!read_full(fd, pdf.data(), pdf.size(), m_cfg.m_requestTimeoutMs)) {
        std::cerr << "[WARN] Failed to read PDF payload (size=" << hdr.pdf_size << ")\n";
        close(fd);
        return;
    }   
    
    std::cerr << "SERVER: PDF payload read fd= "<< fd << ", hdr= "<< hdr.pdf_size << " bytes" << std::endl;

    // QR extraction
    std::string qrResult;
    bool ok = m_qrProcessor.extract(pdf, qrResult);
    std::cerr << "PROCESSOR: Starting QR extraction fd " << fd << " \n";    
    

    if (!ok) {
        std::cerr << "[ERROR] QR extraction failed unexpectedly\n";
        ResponseHeader rh{QrStatus::INTERNAL_ERROR, 0};
        write_full(fd, &rh, sizeof(rh), m_cfg.m_requestTimeoutMs);
        close(fd);
        return;
    }

    // Send response
    ResponseHeader rh{
        QrStatus::OK,
        static_cast<uint32_t>(qrResult.size())
    };

    if (!write_full(fd, &rh, sizeof(rh), m_cfg.m_requestTimeoutMs)) {
        std::cerr << "[WARN] Failed to write response header\n";
        close(fd);
        return;
    }

    if (!qrResult.empty()) {
        if (!write_full(fd, qrResult.data(),
                        qrResult.size(),
                        m_cfg.m_requestTimeoutMs)) {
            std::cerr << "[WARN] Failed to write QR result payload\n";
            close(fd);
            return;
        }
    }
    
    std::cerr << "SERVER: PDF payload write fd= "<< fd << ", qrResult-size= "<< qrResult.size() << " bytes" << std::endl;
    std::cerr << "[DEBUG] Request processed successfully\n";
    close(fd);
}


void ExtractorServer::run()
{
    while (!m_gShutdownRequested.load()) {

        pollfd pfd{m_listenFd, POLLIN, 0};
        int rc = poll(&pfd, 1, 1000);

        if (rc <= 0)
            continue;

        int clientFd = accept(m_listenFd, nullptr, nullptr);
        if (clientFd < 0)
            continue;
        std::cerr << "[DEBUG] SERVER, Client connecte, fd= " << clientFd << std::endl;
        bool enqueued = m_workerPool.enqueue([this, clientFd]() { handleClient(clientFd); });

        if (!enqueued)
        {
            // Overload protection
            close(clientFd);
        }
    }
    std::cerr << "[INFO] Shutdown requested, stopping extractor\n";
    cleanupSocket();
}

}
