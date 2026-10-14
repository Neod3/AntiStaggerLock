// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 ByOcire

#include "LoadGame.h"
#include "Hooks.h"
#include "Settings.h"
#include "StaggerManager.h"
#include "HitTracker.h"

namespace AntiStaggerLock
{
	void EventCallback(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type) {

		case SKSE::MessagingInterface::kDataLoaded:
			{
				SKSE::log::info("Anti Stagger Lock - initializing");

				Settings::GetSingleton();
				StaggerManager::GetSingleton();

				HitEventHandler::Register();
				MagicHitEventHandler::Register();

				StaggerHook::InstallHooks();
				UpdateHook::InstallHook();

				SKSE::log::info("Anti Stagger Lock - ready");
			}
			break;

		case SKSE::MessagingInterface::kNewGame:
		case SKSE::MessagingInterface::kPostLoadGame:
			SKSE::log::debug("Game loaded - stagger system ready");
			break;

		default:
			break;
		}
	}
}
