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

#pragma once
#include "IMessageHandler.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <chrono>

class Dispatcher {
public:
    void registerHandler(const std::string& type,
                         std::unique_ptr<IMessageHandler> handler);

    extractor::ExtractResult dispatch(
        const IMessage& msg,
        uint64_t rid,
        std::chrono::steady_clock::time_point deadline) const;

private:
    std::unordered_map<std::string,
        std::unique_ptr<IMessageHandler>> handlers_;
};
