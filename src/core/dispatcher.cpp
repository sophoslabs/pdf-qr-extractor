#include "core/dispatcher.h"

void Dispatcher::registerHandler(const std::string& type,
                                 std::unique_ptr<IMessageHandler> handler)
{
    handlers_[type] = std::move(handler);
}

std::string Dispatcher::dispatch(const IMessage& msg, uint64_t rid) const
{
    auto it = handlers_.find(msg.getType());

    if (it != handlers_.end()) {
        return it->second->handle(msg, rid);
    }

    return R"({"status":"error","message":"No handler registered"})";
}