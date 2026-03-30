#include "security/security_checks.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <cstring>
#include <stdexcept>
#include <iostream>

namespace extractor::security {

struct ScopedFd {
    int fd;
    explicit ScopedFd(int f) : fd(f) {}
    ~ScopedFd() {
        if (fd >= 0) close(fd);
    }
};

void enforceNotRoot()
{
    if (geteuid() == 0) {
        throw std::runtime_error(
            "PDF QR Extractor must NOT run as root. "
            "Use a dedicated service user.");
    }
}

void validateSocketPath(const std::string& path)
{
    if (path.empty())
        throw std::runtime_error("Socket path is empty");

    if (path.length() >= sizeof(sockaddr_un::sun_path))
        throw std::runtime_error("Socket path too long");

    if (path[0] != '/')
        throw std::runtime_error("Socket path must be absolute");
}

void validateSocketDirectory(const std::string& socketPath)
{
    auto pos = socketPath.find_last_of('/');
    std::string dir;

    if (pos == std::string::npos || pos == 0)
        dir = "/";
    else
        dir = socketPath.substr(0, pos);

    struct stat st{};
    if (stat(dir.c_str(), &st) != 0)
        throw std::runtime_error("Socket directory does not exist");

    if (!S_ISDIR(st.st_mode))
        throw std::runtime_error("Socket path directory invalid");

    if ((st.st_mode & S_IWOTH) && !(st.st_mode & S_ISVTX)) {
        throw std::runtime_error(
            "Socket directory is world-writable and unsafe");
    }
}

void enforceSingleInstance(const std::string& socketPath)
{
    if (access(socketPath.c_str(), F_OK) != 0)
        return;

    ScopedFd sock(socket(AF_UNIX, SOCK_STREAM, 0));
    if (sock.fd < 0)
        return;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;

    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s",
                  socketPath.c_str());

    socklen_t len = offsetof(sockaddr_un, sun_path) +
                    std::strlen(addr.sun_path);

    if (connect(sock.fd, (sockaddr*)&addr, len) == 0) {
        throw std::runtime_error("Extractor already running");
    }

    std::cerr << "[WARN] Stale socket detected, removing: "
              << socketPath << std::endl;

    unlink(socketPath.c_str());
}


void verifyPeerUid(int clientFd, uid_t allowedUid)
{
    struct ucred cred{};
    socklen_t len = sizeof(cred);

    if (getsockopt(clientFd,
                   SOL_SOCKET,
                   SO_PEERCRED,
                   &cred,
                   &len) != 0) {
        throw std::runtime_error("Failed to obtain peer credentials");
    }

    if (cred.uid != allowedUid) {
        std::cerr << "[SECURITY] Unauthorized UID "
                  << cred.uid << " rejected\n";
        throw std::runtime_error("Unauthorized client UID");
    }
}

} // namespace extractor::security
