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

#pragma once

#define ASIO_STANDALONE
#include <asio.hpp>

#include "config/extractor_config.h"
#include "processor/pdf_qr_processor.h"
#include "server/worker_pool.h"
#include "core/dispatcher_factory.h"

#include <atomic>
#include <string>

namespace extractor {

class ExtractorServer {
public:
    explicit ExtractorServer(const ExtractorConfig& cfg);
    ~ExtractorServer() noexcept = default;

    ExtractorServer(const ExtractorServer&) = delete;
    ExtractorServer& operator=(const ExtractorServer&) = delete;

    ExtractorServer(ExtractorServer&&) = default;
    ExtractorServer& operator=(ExtractorServer&&) = default;

    void run();
    void stop();

private:   
    void setupAcceptor();

    using Socket = asio::local::stream_protocol::socket;
    void handleClient(Socket socket);    
   
private:

    ExtractorConfig m_cfg;

    PDFQRProcessor m_qrProcessor;

    WorkerPool m_workerPool;
    Dispatcher m_dispatcher;
  
    asio::io_context m_ioContext;
    asio::local::stream_protocol::acceptor m_acceptor;    
};

} // namespace extractor
