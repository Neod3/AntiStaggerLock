// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 ByOcire

#pragma once
#include "StaggerManager.h"

namespace AntiStaggerLock
{
	// ============================================================
	//  HOOK PROTECTION
	// ============================================================
	namespace HookProtection
	{
		struct VTableBackup
		{
			std::uintptr_t previousPtr = 0;
			std::uintptr_t ourPtr = 0;
			std::size_t vtableIndex = 0;
			std::string vtableName;
		};

		inline std::vector<VTableBackup> installedHooks;

		// Troca a entrada da vtable e devolve o ponteiro anterior (cadeia cooperativa)
		inline std::uintptr_t SafeInstallHook(
			REL::Relocation<std::uintptr_t>& vtbl,
			std::size_t index,
			std::uintptr_t hookAddress,
			const std::string& name)
		{
			auto actualPrevious = vtbl.write_vfunc(index, hookAddress);

			VTableBackup backup;
			backup.previousPtr = actualPrevious;
			backup.ourPtr = hookAddress;
			backup.vtableIndex = index;
			backup.vtableName = name;
			installedHooks.push_back(backup);

			SKSE::log::info("Hook installed: {} at index {} (previous: 0x{:X})",
				name, index, actualPrevious);

			return actualPrevious;
		}

		inline void LogHookSummary()
		{
			SKSE::log::info("Total: {} hooks installed", installedHooks.size());
		}
	}

	// ============================================================
	//  STAGGER HOOK (NPCs + Player)
	// ============================================================
	class StaggerHook
	{
	public:
		static void InstallHooks();

		using NotifyFn = bool (*)(RE::IAnimationGraphManagerHolder*, const RE::BSFixedString&);

	private:
		class CharacterEx
		{
		public:
			static void InstallHook();

		private:
			static bool Hook_NotifyAnimationGraph(
				RE::IAnimationGraphManagerHolder* a_holder,
				const RE::BSFixedString& a_eventName);

			static inline NotifyFn _PreviousNotify = nullptr;
		};

		class PlayerEx
		{
		public:
			static void InstallHook();

		private:
			static bool Hook_NotifyAnimationGraph(
				RE::IAnimationGraphManagerHolder* a_holder,
				const RE::BSFixedString& a_eventName);

			static inline NotifyFn _PreviousNotify = nullptr;
		};

		friend class CharacterEx;
		friend class PlayerEx;

		// true = bloquear o evento
		static bool ProcessStaggerEvent(
			RE::IAnimationGraphManagerHolder* a_holder,
			const RE::BSFixedString& a_eventName);
	};

	// ============================================================
	//  UPDATE HOOK (limpeza periódica do cache)
	// ============================================================
	class UpdateHook
	{
	public:
		static void InstallHook();

		using UpdateFn = void (*)(RE::PlayerCharacter*, float);

	private:
		class PlayerEx
		{
		public:
			static void InstallHook();

		private:
			static void Hook_Update(RE::PlayerCharacter* a_this, float a_delta);

			static inline UpdateFn _PreviousUpdate = nullptr;
			static inline float cleanupTimer = 0.0f;

			static inline constexpr float CLEANUP_INTERVAL = 30.0f;
		};

		friend class PlayerEx;
	};
}
