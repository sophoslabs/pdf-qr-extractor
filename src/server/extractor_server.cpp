#define ASIO_STANDALONE
#include <asio.hpp>

#include "server/extractor_server.h"
#include "core/dispatcher_factory.h"
#include "core/messages.h"
#include "security/security_checks.h"
#include "system/shutdown.h"
#include "protocol/request_validation.h"
#include "protocol/pdf_qr_protocol.h"

#include "utils/asio_io_utils.h"
#include "utils/response_utils.h"
#include "logging/logger.h"


#include <iostream>

namespace extractor {

using asio::local::stream_protocol;
using namespace extractor::utils;
using namespace extractor::asio_utils;
using namespace extractor::security;
using namespace extractor::protocol;

ExtractorServer::ExtractorServer(const ExtractorConfig& cfg)
    : m_cfg(cfg),
      m_qrProcessor(cfg),
      m_workerPool(cfg.m_workerThreads),
      m_dispatcher(createDispatcher(m_qrProcessor)),
      m_ioContext(),
      m_acceptor(m_ioContext)
{
    enforceNotRoot();
    validateSocketPath(m_cfg.m_socketPath);
    validateSocketDirectory(m_cfg.m_socketPath);
    enforceSingleInstance(m_cfg.m_socketPath);
    setupAcceptor();
}

void ExtractorServer::setupAcceptor()
{
    ::unlink(m_cfg.m_socketPath.c_str());

    stream_protocol::endpoint ep(m_cfg.m_socketPath);

    m_acceptor.open(ep.protocol());
    m_acceptor.bind(ep);
    m_acceptor.listen();
}

void ExtractorServer::run()
{
    while (!m_gShutdownRequested.load())
    {
        stream_protocol::socket socket(m_ioContext);

        asio::error_code ec;

        m_acceptor.accept(socket, ec);

        if (ec)
        {
            if (m_gShutdownRequested.load())
            {
                break; // graceful exit
            }
            
            LOG_ERROR ("SERVER",  std::string("[WARN] accept failed: ") + ec.message());
            continue;
        }

       
        if (m_gShutdownRequested.load())
        {
            socket.close();
            break;
        }
        

        auto sockPtr = std::make_shared<stream_protocol::socket>(std::move(socket));

        bool enqueued = m_workerPool.enqueue(
            [this, sockPtr]() mutable
            {
                handleClient(std::move(*sockPtr));
            });

        if (!enqueued)
        {            
            LOG_WARN ("SERVER", "[WARN] worker queue full, dropping connection");
            sockPtr->close();
        }
    }
}

void ExtractorServer::handleClient(stream_protocol::socket socket)
{
    try {
        int fd = socket.native_handle();

        verifyPeerUid(fd, m_cfg.m_allowedUid);

        RequestHeader hdr{};

        
        if (!read_full(socket, asio::buffer(&hdr, sizeof(hdr)), m_cfg.m_requestTimeoutMs)) {
            return;
        }

        uint64_t rid = hdr.rid;
       
        QrStatus st = validateRequest(hdr, m_cfg);
        if (st != QrStatus::OK) {
            send_error(socket, st, m_cfg.m_requestTimeoutMs);            
            return;
        }

        std::string pdf(hdr.pdf_size, '\0');
        auto s2 = std::chrono::steady_clock::now();
        if (!read_full(socket, asio::buffer(pdf.data(), pdf.size()), m_cfg.m_requestTimeoutMs)) {
            return;
        }        
        auto e2 = std::chrono::steady_clock::now();
        LOG_ERROR("SERVER and PROCESSOR", "RID=" + std::to_string(rid) + "PROCESSOR: PDF payload read= " + std::to_string(fd) +  ", rtime_ms= " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(e2 - s2).count()) + " ms"); //to be removed

        std::string qrResult;
        auto start = std::chrono::steady_clock::now(); // to be removed
        try {
            JsonMessage msg("pdf", pdf);
            qrResult = m_dispatcher.dispatch(msg, rid);
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] Processing failed: " << e.what() << "\n"; 
            LOG_ERROR ("SERVER",  std::string("Processing failed: ") + e.what());      
            send_error(socket, QrStatus::INTERNAL_ERROR, m_cfg.m_requestTimeoutMs);
            return;
        }
        auto end = std::chrono::steady_clock::now(); // to be removed
        LOG_ERROR("SERVER and PROCESSOR", "RID=" + std::to_string(rid) + "Extraction finished fd= " + std::to_string(fd) +  ", time_ms= " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) + " ms"); //to be removed

        ResponseHeader rh{
            QrStatus::OK,
            static_cast<uint32_t>(qrResult.size())
        };
        
        if (!write_full(socket,
                        asio::buffer(&rh, sizeof(rh)),
                        m_cfg.m_requestTimeoutMs)) {
            return;
        }
        auto s1 = std::chrono::steady_clock::now(); // to be removed
        if (!qrResult.empty()) {
            write_full(socket, asio::buffer(qrResult.data(), qrResult.size()), m_cfg.m_requestTimeoutMs);
        }
        auto e1 = std::chrono::steady_clock::now();
        LOG_ERROR("SERVER and PROCESSOR", "RID=" + std::to_string(rid) +  "PDF payload write fd= " + std::to_string(fd) +  ", wtime_ms= " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(e1 - s1).count()) + " ms"); //to be removed
    }
    catch (const std::exception& e) {
        std::cerr << "[ERROR] Connection error: " << e.what() << "\n";
        LOG_ERROR ("SERVER", "[ERROR] Connection error: ");
    }
}

void ExtractorServer::stop()
{
    m_gShutdownRequested.store(true);

    asio::error_code ec;
    m_acceptor.cancel(ec);
    m_acceptor.close(ec);

    ::unlink(m_cfg.m_socketPath.c_str());
}

} // namespace extractor
