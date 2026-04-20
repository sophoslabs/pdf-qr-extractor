// Copyright (C) 2026 Sophos Limited
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This file is part of pdf-qr-extractor.
//
// pdf-qr-extractor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// pdf-qr-extractor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with pdf-qr-extractor. If not, see <https://www.gnu.org/licenses/>

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
            LOG_WARN("SERVER", "[WARN] worker queue full, sending NOT_AVAILABLE");
            ResponseHeader hdr{QrStatus::NOT_AVAILABLE, 0};
            asio::error_code writeEc;
            asio::write(*sockPtr, asio::buffer(&hdr, sizeof(hdr)), writeEc);
            sockPtr->close();
        }
    }
}

void ExtractorServer::handleClient(stream_protocol::socket socket)
{
    asio::io_context io;
    stream_protocol::socket sock(io);

    bool responseSent = false;

    auto reqStart = std::chrono::steady_clock::now();
    auto reqTimeout = std::chrono::milliseconds(m_cfg.m_requestTimeoutMs);

    // Deadline (same as request timeout for now)
    auto deadline = reqStart + reqTimeout;

    auto isTimedOut = [&]() {
        return std::chrono::steady_clock::now() > deadline;
    };

    auto trySendError = [&]() {
        if (!responseSent) {
            try {
                send_error(io, sock, QrStatus::INTERNAL_ERROR, m_cfg.m_requestTimeoutMs);
            } catch (...) {
                LOG_DEBUG("SERVER", "send_error failed (client likely disconnected)");
            }
        }
    };

    try {
        // Rebind socket
        sock.assign(stream_protocol(), socket.native_handle());
        socket.release();

        int fd = sock.native_handle();
        verifyPeerUid(fd, m_cfg.m_allowedUid);

        RequestHeader hdr{};

        // READ HEADER
        if (!read_full(io, sock,
                       asio::buffer(&hdr, sizeof(hdr)),
                       m_cfg.m_requestTimeoutMs)) {
            //LOG_DEBUG("SERVER", "Header read failed: client disconnected or timed out before sending header");  //noisy log, can be normal client behavior.
            return;
        }

        if (isTimedOut()) {
            LOG_ERROR("SERVER", "Request timeout before processing");
            send_error(io, sock, QrStatus::TIMEOUT, m_cfg.m_requestTimeoutMs);
            return;
        }

        uint64_t rid = hdr.rid;

        // VALIDATION
        QrStatus st = validateRequestHeader(hdr, m_cfg);
        if (st != QrStatus::OK) {
            send_error(io, sock, st, m_cfg.m_requestTimeoutMs);
            return;
        }

        // READ PDF
        std::string pdf(hdr.pdf_size, '\0');

        if (!read_full(io, sock,
                       asio::buffer(pdf.data(), pdf.size()),
                       m_cfg.m_requestTimeoutMs)) {
            LOG_DEBUG("SERVER", "PDF Header read timeout or client disconnected");
            return;
        }

        if (isTimedOut()) {
            LOG_ERROR("SERVER", "Request timeout before dispatch");
            send_error(io, sock, QrStatus::TIMEOUT, m_cfg.m_requestTimeoutMs);
            return;
        }
        
        // PROCESS (deadline-aware)
        ExtractResult res;

        try {
            JsonMessage msg("pdf", pdf);
            
            res = m_dispatcher.dispatch(msg, rid, deadline);
        }
        catch (const std::exception& e) {
            LOG_ERROR("SERVER", std::string("Processing failed: ") + e.what());
            send_error(io, sock, QrStatus::INTERNAL_ERROR, m_cfg.m_requestTimeoutMs);
            return;
        }
        catch (...) {
            LOG_ERROR("SERVER", "Unknown processing error");
            send_error(io, sock, QrStatus::INTERNAL_ERROR, m_cfg.m_requestTimeoutMs);
            return;
        }
  
        // HANDLE RESULT
        if (isTimedOut()) {
            LOG_ERROR("SERVER", "Request exceeded total timeout after processing");
            send_error(io, sock, QrStatus::TIMEOUT, m_cfg.m_requestTimeoutMs);
            return;
        }

        switch (res.status)
        {

        case ExtractStatus::OK:
        {
            ResponseHeader rh{
                QrStatus::OK,
                static_cast<uint32_t>(res.data.size())};

            if (!write_full(io, sock,
                            asio::buffer(&rh, sizeof(rh)),
                            m_cfg.m_requestTimeoutMs))
            {
                LOG_ERROR("SERVER", "Header write failed");
                return;
            }

            if (!res.data.empty())
            {
                if (!write_full(io, sock,
                                asio::buffer(res.data.data(), res.data.size()),
                                m_cfg.m_requestTimeoutMs))
                {
                    LOG_ERROR("SERVER", "Body write failed");
                    return;
                }
            }

            responseSent = true;
            break;
        }
        case ExtractStatus::NO_QR:
        {
            ResponseHeader rh{QrStatus::NO_QR, 0};
            if (!write_full(io, sock, asio::buffer(&rh, sizeof(rh)), m_cfg.m_requestTimeoutMs))
            {
                LOG_ERROR("SERVER", "NO_QR header write failed");
                return;
            }
            responseSent = true;
            break;
        }
        case ExtractStatus::INVALID_INPUT:
            send_error(io, sock, QrStatus::INVALID_REQUEST, m_cfg.m_requestTimeoutMs);
            responseSent = true;
            return;

        case ExtractStatus::PROCESSING_ERROR:
            send_error(io, sock, QrStatus::INTERNAL_ERROR, m_cfg.m_requestTimeoutMs);
            responseSent = true;
            return;

        case ExtractStatus::TIMEOUT:
            send_error(io, sock, QrStatus::TIMEOUT, m_cfg.m_requestTimeoutMs);
            responseSent = true;
            return;
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("SERVER", std::string("Connection error: ") + e.what());
        trySendError();
    }
    catch (...) {
        LOG_ERROR("SERVER", "Unknown connection error");
        trySendError();
    }
     LOG_INFO("SERVER", "Request processed successfully");
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
