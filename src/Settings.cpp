// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 ByOcire

#include "Settings.h"

namespace AntiStaggerLock
{
	static auto MainConfig = COMPILE_PROXY("AntiStaggerLock.ini"sv);

	Settings::Settings()
	{
		// Defaults iguais aos da página do mod no Nexus
		MainConfig.Bind<0.0, 10.0>(StaggerCooldownTime, 3.5);
		MainConfig.Bind<0.0, 10.0>(MagicStaggerCooldown, 5.0);
		MainConfig.Bind<0.0, 1.0>(MaxMagnitudeToBlock, 0.85);
		MainConfig.Bind(EnableAntiStagger, true);
		MainConfig.Bind(ApplyToPlayer, true);
		MainConfig.Bind(ApplyToNPCs, true);
		MainConfig.Bind(ApplyToCreatures, true);

		MainConfig.Load();

		INFO("=== Anti Stagger Lock Settings ===");
		INFO("Config file: {}"sv, MainConfig.get_parser().filepath());
		PrintSettingValue(StaggerCooldownTime);
		PrintSettingValue(MagicStaggerCooldown);
		PrintSettingValue(MaxMagnitudeToBlock);
		PrintSettingValue(EnableAntiStagger);
		PrintSettingValue(ApplyToPlayer);
		PrintSettingValue(ApplyToNPCs);
		PrintSettingValue(ApplyToCreatures);
		INFO("==================================");
	}
}
