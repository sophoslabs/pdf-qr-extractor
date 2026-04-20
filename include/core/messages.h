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
#include <string>

class IMessage {
public:
    virtual ~IMessage() = default;
    virtual std::string getType() const = 0;
    virtual const std::string& raw() const = 0;
};

class JsonMessage : public IMessage {
public:
    JsonMessage(std::string type, std::string raw)
        : type_(std::move(type)), raw_(std::move(raw)) {}

    std::string getType() const override {
        return type_;
    }

    const std::string& raw() const override {
        return raw_;
    }

private:
    std::string type_;
    std::string raw_;
};