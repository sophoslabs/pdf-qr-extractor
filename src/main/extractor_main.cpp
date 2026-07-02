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

#include "config/extractor_config.h"
#include "server/extractor_server.h"
#include "system/signal_handler.h"
#include "system/shutdown.h"
#include "misc/version.h"
#include "logging/logger.h"
#include "utils/poppler_utils.h"
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

        // Validate config path: must be absolute and free of traversal sequences
        if (configPath.empty() || configPath[0] != '/') {
            throw std::runtime_error("Config path must be an absolute path");
        }
        if (configPath.find("..") != std::string::npos) {
            throw std::runtime_error("Config path must not contain '..' components");
        }

        // Load configuration
        ExtractorConfig cfg = loadExtractorConfig(configPath);

        g_socketPath = cfg.m_socketPath;

        install_signal_handlers();

        extractor::utils::initPoppler(); // Suppress Poppler error messages globally
        
        Logger::instance().init(parseLogLevel(cfg.m_logLevel), cfg.m_consoleOutput, cfg.m_logFile, cfg.m_logSize, cfg.m_maxLogFiles);

          // STARTUP LOGS
        std::cerr << "============================================================" << std::endl;
        std::cerr << " PDF QR Extractor starting" << std::endl;
        std::cerr << " Version : " << BUILD_VERSION << std::endl;
        std::cerr << " Build   : " << __DATE__ << " " << __TIME__ << std::endl;
        std::cerr << " Config  : " << configPath << std::endl;
        std::cerr << " EXTRACTOR_SOCKET_PATH : " << cfg.m_socketPath << std::endl;
        std::cerr << " WORKER_THREADS : " << cfg.m_workerThreads << std::endl;
        std::cerr << " MAX_PDF_QR_DECODE_KB_SIZE  : " << (cfg.m_maxPdfSizeBytes / 1024) << std::endl;
        std::cerr << " MAX_QR_DECODE_KB_SIZE  : " << (cfg.m_maxQrImageBytes / 1024) << std::endl;
        std::cerr << " MAX_PDF_PAGES_FOR_QR_SCAN : " << cfg.m_maxPagesToScan << std::endl;
        std::cerr << " ALLOWED_UID : " << cfg.m_allowedUid << std::endl;
        std::cerr << " REQUEST_TIMEOUT_MS : " << cfg.m_requestTimeoutMs << std::endl;
        std::cerr << " CONSOLE_OUTPUT : " << cfg.m_consoleOutput << std::endl;
        std::cerr << " LOG_SIZE : " << (cfg.m_logSize / (1024 * 1024)) << " MB" << std::endl;
        std::cerr << " LOG_FILE : " << cfg.m_logFile << std::endl;
        std::cerr << " LOG_LEVEL : " << cfg.m_logLevel << std::endl;
        std::cerr << "============================================================" << std::endl;        

        // Start server
        ExtractorServer server(cfg);

        LOG_INFO("MAIN", "Extractor started successfully");

        std::atomic<bool> serverAlive{true};

        std::thread shutdownWatcher([&]()
                                    {
                                        while (!m_gShutdownRequested.load())
                                        {
                                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                        }

                                        if (serverAlive.load())
                                            server.stop();
                                    });

        try {
            server.run();
        }
        catch (...) {
            serverAlive.store(false);
            shutdownWatcher.join();
            throw;
        }

        serverAlive.store(false);

        LOG_INFO("MAIN", "Extractor shutting down cleanly");

        shutdownWatcher.join();

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
