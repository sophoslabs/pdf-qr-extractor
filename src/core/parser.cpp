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

// NOTE: Parser is not currently used. The dispatcher handles message routing
// directly via request_validation. This file is retained for potential future use.
#include "core/parser.h"
#include <cctype>

static constexpr size_t MAX_INPUT_SIZE = 4096;

std::unique_ptr<IMessage> Parser::parse(const std::string& input)
{
    if (input.empty() || input.size() > MAX_INPUT_SIZE)
        return nullptr;

    std::string type = extractType(input);

    if (type.empty())
        return nullptr;

    return std::make_unique<JsonMessage>(type, input);
}

std::string Parser::extractType(const std::string& json)
{
    const size_t n = json.size();
    size_t i = 0;

    while (i < n) {
        auto pos = json.find("\"type\"", i);
        if (pos == std::string::npos)
            return "";

        // "type" is only a valid key if preceded (ignoring whitespace) by '{' or ','
        bool validKey = false;
        if (pos > 0) {
            size_t back = pos - 1;
            while (back > 0 && std::isspace((unsigned char)json[back]))
                --back;
            char preceding = json[back];
            validKey = (preceding == '{' || preceding == ',');
        }

        if (!validKey) {
            i = pos + 6;
            continue;
        }

        size_t cur = pos + 6; // past `"type"`

        while (cur < n && std::isspace((unsigned char)json[cur])) ++cur;
        if (cur >= n || json[cur] != ':') return "";
        ++cur;
        while (cur < n && std::isspace((unsigned char)json[cur])) ++cur;
        if (cur >= n || json[cur] != '"') return "";
        ++cur;

        std::string value;
        while (cur < n) {
            if (json[cur] == '\\' && cur + 1 < n) { cur += 2; continue; }
            if (json[cur] == '"') break;
            value += json[cur++];
        }

        if (cur >= n || json[cur] != '"') return "";
        return value;
    }

    return "";
}