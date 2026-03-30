#pragma once
#include <string>
#include <sys/types.h>

namespace extractor::security {

void enforceNotRoot();
void validateSocketPath(const std::string& path);
void validateSocketDirectory(const std::string& path);
void enforceSingleInstance(const std::string& socketPath);
void verifyPeerUid(int clientFd, uid_t allowedUid);

}
