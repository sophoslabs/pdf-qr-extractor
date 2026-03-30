#include "core/parser.h"

std::unique_ptr<IMessage> Parser::parse(const std::string& input)
{
    if (input.empty()) {
        return nullptr;
    }

    std::string type = extractType(input);

    if (type.empty()) {
        return nullptr;
    }

    return std::make_unique<JsonMessage>(type, input);
}

std::string Parser::extractType(const std::string& json)
{
    // VERY SIMPLE extraction:
    // looks for: "type":"xyz"

    const std::string key = "\"type\"";
    auto pos = json.find(key);
    if (pos == std::string::npos) return "";

    pos = json.find(":", pos);
    if (pos == std::string::npos) return "";

    pos = json.find("\"", pos);
    if (pos == std::string::npos) return "";

    auto end = json.find("\"", pos + 1);
    if (end == std::string::npos) return "";

    return json.substr(pos + 1, end - pos - 1);
}