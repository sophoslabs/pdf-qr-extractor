#pragma once
#include <cstddef>

bool read_full(int fd, void* buf, size_t len, int timeoutMs);
bool write_full(int fd, const void* buf, size_t len, int timeoutMs);
