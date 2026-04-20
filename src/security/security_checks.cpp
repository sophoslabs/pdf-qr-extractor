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

#include "security/security_checks.h"
#include "logging/logger.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <cstring>
#include <stdexcept>

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
    if (lstat(dir.c_str(), &st) != 0)
        throw std::runtime_error("Socket directory does not exist");

    if (S_ISLNK(st.st_mode))
        throw std::runtime_error("Socket directory must not be a symbolic link");

    if (!S_ISDIR(st.st_mode))
        throw std::runtime_error("Socket path directory invalid");

    if (st.st_mode & S_IWOTH) {
        throw std::runtime_error(
            "Socket directory is world-writable and unsafe");
    }
}

void enforceSingleInstance(const std::string& socketPath)
{
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

    LOG_WARN("SECURITY", "Stale socket detected, removing: " + socketPath);

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
        LOG_WARN("SECURITY", "Unauthorized UID " + std::to_string(cred.uid) + " rejected");
        throw std::runtime_error("Unauthorized client UID");
    }
}

} // namespace extractor::security
