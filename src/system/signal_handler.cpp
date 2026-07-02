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

#include "system/signal_handler.h"
#include "system/shutdown.h"
#include <signal.h>
#include <stdexcept>

static void handle_signal(int)
{
    m_gShutdownRequested.store(true);
}

void install_signal_handlers()
{
    struct sigaction sa{};
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT,  &sa, nullptr) != 0 ||
        sigaction(SIGTERM, &sa, nullptr) != 0 ||
        sigaction(SIGQUIT, &sa, nullptr) != 0)
    {
        throw std::runtime_error("Failed to install signal handlers");
    }

    // Ignore SIGPIPE globally
    struct sigaction pipe_sa{};
    pipe_sa.sa_handler = SIG_IGN;
    sigemptyset(&pipe_sa.sa_mask);

    if (sigaction(SIGPIPE, &pipe_sa, nullptr) != 0)
    {
        throw std::runtime_error("Failed to ignore SIGPIPE");
    }
}
