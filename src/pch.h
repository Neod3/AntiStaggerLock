// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 ByOcire

#pragma once

// DKUtil só define CONFIG_ENTRY se SKSEAPI existir, e o CommonLibSSE-NG v8.4.1
// não define. Sem isto o INI vai para a raiz do jogo. Vem antes do DKUtil.
#define CONFIG_ENTRY "Data\\SKSE\\Plugins\\"

// Evita colisão com macros do Windows (min/max/MAX_PATH).
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <SKSE/SKSE.h>

#ifdef MAX_PATH
#	undef MAX_PATH
#endif
#ifdef min
#	undef min
#endif
#ifdef max
#	undef max
#endif

#include <RE/Skyrim.h>

// DKUtil::Logger usa Plugin::NAME/AUTHOR, então version.h entra aqui.
#include "version.h"
