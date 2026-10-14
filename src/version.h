// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 ByOcire

#pragma once

#include <string_view>
using namespace std::literals::string_view_literals;  // sufixo "..."sv

namespace Plugin
{
	inline constexpr auto NAME    = "AntiStaggerLock"sv;
	inline constexpr auto AUTHOR  = "ByOcire"sv;
	inline constexpr REL::Version VERSION{ 1, 1, 0, 0 };
}
