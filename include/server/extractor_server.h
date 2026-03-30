#pragma once

#define ASIO_STANDALONE
#include <asio.hpp>

#include "config/extractor_config.h"
#include "processor/pdf_qr_processor.h"
#include "server/worker_pool.h"
#include "core/dispatcher_factory.h"
#include "core/parser.h"

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
    Parser m_parser;
    Dispatcher m_dispatcher;
  
    asio::io_context m_ioContext;
    asio::local::stream_protocol::acceptor m_acceptor;    
};

} // namespace extractor
