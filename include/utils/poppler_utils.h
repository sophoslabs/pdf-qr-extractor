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

/* Poppler utility file to suppress error messages during runtime */

#pragma once
#include <poppler/cpp/poppler-global.h>

namespace extractor::utils {

inline void suppressPopplerErrors(const std::string& msg, void* data)
{
    (void)msg;
    (void)data;
}

inline void initPoppler()
{
    poppler::set_debug_error_function(suppressPopplerErrors, nullptr);
}

}