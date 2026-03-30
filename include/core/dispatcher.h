#pragma once
#include "IMessageHandler.h"
#include <unordered_map>
#include <memory>
#include <string>

class Dispatcher {
public:
    void registerHandler(const std::string& type,
                         std::unique_ptr<IMessageHandler> handler);

    std::string dispatch(const IMessage& msg, uint64_t rid) const;

private:
    std::unordered_map<std::string,
        std::unique_ptr<IMessageHandler>> handlers_;
};