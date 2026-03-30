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
        // Create per-request io_context
        asio::io_context io;

        // Rebind socket to this io_context
        stream_protocol::socket sock(io);
        sock.assign(stream_protocol(), socket.native_handle());
        socket.release();   // VERY IMPORTANT

        int fd = sock.native_handle();

        verifyPeerUid(fd, m_cfg.m_allowedUid);

        RequestHeader hdr{};

        if (!read_full(io, sock,
                       asio::buffer(&hdr, sizeof(hdr)),
                       m_cfg.m_requestTimeoutMs)) {
            LOG_ERROR("SERVER", "Header read operation failed, timeout= " + std::to_string(m_cfg.m_requestTimeoutMs)+ " milliseconds");
            return;
        }

        uint64_t rid = hdr.rid;

        QrStatus st = validateRequest(hdr, m_cfg);
        if (st != QrStatus::OK) {
            send_error(io, sock, st, m_cfg.m_requestTimeoutMs);
            return;
        }

        std::string pdf(hdr.pdf_size, '\0');        

        if (!read_full(io, sock,
                       asio::buffer(pdf.data(), pdf.size()),
                       m_cfg.m_requestTimeoutMs)) {
            LOG_ERROR("SERVER", "PDF data read operation failed, timeout= " + std::to_string(m_cfg.m_requestTimeoutMs) + " milliseconds");
            return;
        }
        

        std::string qrResult;
        
        try {
            JsonMessage msg("pdf", pdf);
            qrResult = m_dispatcher.dispatch(msg, rid);
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] Processing failed: " << e.what() << "\n"; 
            LOG_ERROR ("SERVER",  std::string("Processing failed: ") + e.what());
            send_error(io, socket, QrStatus::INTERNAL_ERROR, m_cfg.m_requestTimeoutMs);
            return;
        }
        
        ResponseHeader rh{
            QrStatus::OK,
            static_cast<uint32_t>(qrResult.size())
        };
        
        if (!write_full(io, socket,
                        asio::buffer(&rh, sizeof(rh)),
                        m_cfg.m_requestTimeoutMs)) {
            return;
        }
        
        if (!qrResult.empty()) {
            write_full(io, socket, asio::buffer(qrResult.data(), qrResult.size()), m_cfg.m_requestTimeoutMs);
        }
        
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
