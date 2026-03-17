#include "config/extractor_config.h"
#include "server/extractor_server.h"
#include "system/signal_handler.h"
#include "version.h"
#include <iostream>
#include <csignal>
#include <unistd.h>

using namespace extractor;

static std::string g_socketPath;

int main(int argc, char* argv[])
{
    try {

        if (argc < 2) {
            throw std::runtime_error(
                "Usage: extractor <config_file> [override_config_file]");
        }

        std::string configPath = argv[1];

        if (argc >= 3)
            configPath = argv[2];

        // Load configuration
        ExtractorConfig cfg = loadExtractorConfig(configPath);

        g_socketPath = cfg.m_socketPath;

        install_signal_handlers();

        // STARTUP LOGS
        std::cerr << "========================================" << std::endl;
        std::cerr << " PDF QR Extractor starting" << std::endl;
        std::cerr << " Version : v1.0" << std::endl;
        std::cerr << " Build   : " << __DATE__ << " " << __TIME__ << std::endl;
        std::cerr << " Config  : " << configPath << std::endl;
        std::cerr << " EXTRACTOR_SOCKET_PATH : " << cfg.m_socketPath << std::endl;
        std::cerr << " WORKER_THREADS : " << cfg.m_workerThreads << std::endl;
        std::cerr << " MAX_PDF_QR_DECODE_KB_SIZE  : " << (cfg.m_maxPdfSizeBytes / 1000) << std::endl;
        std::cerr << " MAX_PDF_PAGES_FOR_QR_SCAN : " << cfg.m_maxPagesToScan << std::endl;
        std::cerr <<" ALLOWED_UID : " << cfg.m_allowedUid << std::endl;
        std::cerr << "========================================" << std::endl;

        // Start server
        ExtractorServer server(cfg);

        std::cerr << "Extractor server starting event loop" << std::endl;
        std::cerr << "Extractor started Successfully" << std::endl;

        server.run();

        std::cerr << "Extractor shutting down cleanly"
                  << std::endl;

        return 0;
    }
    catch (const std::exception& ex) {

        std::cerr << "Extractor startup failed: "
                  << ex.what() << std::endl;

        if (!g_socketPath.empty())
            unlink(g_socketPath.c_str());

        return 1;
    }
}
