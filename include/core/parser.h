#pragma once
#include "messages.h"
#include <memory>
#include <string>

class Parser {
public:
    std::unique_ptr<IMessage> parse(const std::string& input);

private:
    std::string extractType(const std::string& json);
};