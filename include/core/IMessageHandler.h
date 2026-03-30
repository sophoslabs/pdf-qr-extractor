#pragma once
#include "messages.h"
#include <string>

class IMessageHandler {
public:
    virtual ~IMessageHandler() = default;
    virtual std::string handle(const IMessage& msg, uint64_t rid) = 0;
};