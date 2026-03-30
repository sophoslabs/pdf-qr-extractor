#include "system/shutdown.h"
#include <signal.h>
#include <atomic>

std::atomic<bool> m_gShutdownRequested{false};

static void handleSignal(int)
{
    m_gShutdownRequested.store(true);
}