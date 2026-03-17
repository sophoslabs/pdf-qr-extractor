#include "system/signal_handler.h"
#include "system/shutdown.h"
#include <signal.h>

static void handle_signal(int)
{
    m_gShutdownRequested.store(true);
}

void install_signal_handlers()
{
    struct sigaction sa{};
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGINT,  &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGQUIT, &sa, nullptr);

    // Ignore SIGPIPE globally
    struct sigaction pipe_sa{};
    pipe_sa.sa_handler = SIG_IGN;
    sigemptyset(&pipe_sa.sa_mask);

    sigaction(SIGPIPE, &pipe_sa, nullptr);
}
