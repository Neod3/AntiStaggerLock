// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 ByOcire

#pragma once
#include "DKUtil/Config.hpp"
#include "DKUtil/Utility.hpp"

namespace AntiStaggerLock
{
	using namespace DKUtil::Alias;

	class Settings : public DKUtil::model::Singleton<Settings>
	{
		friend DKUtil::model::Singleton<Settings>;

	private:
		Settings();

		template <class T>
		static void PrintSettingValue(const T& a_setting)
		{
			INFO("Setting:\"{}\" is {}"sv, a_setting.get_key(), a_setting.get_data());
		}

	public:
		// Cooldown (s) entre staggers físicos (melee/projétil)
		Double StaggerCooldownTime{ "StaggerCooldownTime", "AntiStagger" };

		// Cooldown (s) entre staggers mágicos
		Double MagicStaggerCooldown{ "MagicStaggerCooldown", "AntiStagger" };

		// Magnitude máxima (0.0-1.0) que o cooldown físico ainda bloqueia.
		// Acima disso é golpe forte: ignora o cooldown atual, mas inicia um novo.
		// Magia sempre respeita o cooldown, qualquer que seja a magnitude.
		//   1.0  = proteção máxima (todo golpe físico pode ser bloqueado)
		//   0.85 = padrão (fracos/médios bloqueados, fortes passam)
		//   0.0  = nenhum bloqueio físico
		Double MaxMagnitudeToBlock{ "MaxMagnitudeToBlock", "AntiStagger" };

		Boolean EnableAntiStagger{ "EnableAntiStagger", "AntiStagger" };
		Boolean ApplyToPlayer{ "ApplyToPlayer", "AntiStagger" };
		Boolean ApplyToNPCs{ "ApplyToNPCs", "AntiStagger" };
		Boolean ApplyToCreatures{ "ApplyToCreatures", "AntiStagger" };
	};
}
