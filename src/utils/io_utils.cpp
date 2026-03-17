#include "utils/io_utils.h"
#include <poll.h>
#include <unistd.h>
#include <errno.h>

static bool wait_fd(int fd, short events, int timeoutMs)
{
    struct pollfd pfd{};
    pfd.fd = fd;
    pfd.events = events;

    int rc = poll(&pfd, 1, timeoutMs);
    return rc > 0 && (pfd.revents & events);
}

bool read_full(int fd, void* buf, size_t len, int timeoutMs)
{
    size_t done = 0;
    char* p = static_cast<char*>(buf);

    while (done < len) {
        if (!wait_fd(fd, POLLIN, timeoutMs))
            return false;

        ssize_t rc = ::read(fd, p + done, len - done);
        if (rc <= 0)
            return false;

        done += rc;
    }
    return true;
}

bool write_full(int fd, const void* buf, size_t len, int timeoutMs)
{
    size_t done = 0;
    const char* p = static_cast<const char*>(buf);

    while (done < len) {
        if (!wait_fd(fd, POLLOUT, timeoutMs))
            return false;

        ssize_t rc = ::write(fd, p + done, len - done);
        if (rc <= 0)
            return false;

        done += rc;
    }
    return true;
}
