// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 ByOcire

#include "Hooks.h"

namespace AntiStaggerLock
{
	namespace EventStrings
	{
		static const RE::BSFixedString staggerStart("staggerStart");
		static const RE::BSFixedString recoilStart("recoilStart");
	}

	// ============================================================
	//  STAGGER HOOK
	// ============================================================

	void StaggerHook::InstallHooks()
	{
		SKSE::log::info("Installing vtable hooks (chained)");

		CharacterEx::InstallHook();
		PlayerEx::InstallHook();

		HookProtection::LogHookSummary();
	}

	void StaggerHook::CharacterEx::InstallHook()
	{
		REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_Character[3] };

		auto previousAddr = HookProtection::SafeInstallHook(
			vtbl,
			0x01,
			reinterpret_cast<std::uintptr_t>(&Hook_NotifyAnimationGraph),
			"VTABLE_Character[3]::NotifyAnimationGraph");

		_PreviousNotify = reinterpret_cast<NotifyFn>(previousAddr);

		if (!_PreviousNotify) {
			SKSE::log::error("Failed to get previous function pointer for Character");
		}
	}

	void StaggerHook::PlayerEx::InstallHook()
	{
		REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_PlayerCharacter[3] };

		auto previousAddr = HookProtection::SafeInstallHook(
			vtbl,
			0x01,
			reinterpret_cast<std::uintptr_t>(&Hook_NotifyAnimationGraph),
			"VTABLE_PlayerCharacter[3]::NotifyAnimationGraph");

		_PreviousNotify = reinterpret_cast<NotifyFn>(previousAddr);

		if (!_PreviousNotify) {
			SKSE::log::error("Failed to get previous function pointer for Player");
		}
	}

	// Evento bloqueado devolve false e não segue pela cadeia (igual ao Break-Stagger);
	// devolver true diria ao chamador que o stagger começou de verdade.
	bool StaggerHook::CharacterEx::Hook_NotifyAnimationGraph(
		RE::IAnimationGraphManagerHolder* a_holder,
		const RE::BSFixedString& a_eventName)
	{
		if (!_PreviousNotify) {
			SKSE::log::error("CharacterEx: _PreviousNotify is null");
			return false;
		}

		if (ProcessStaggerEvent(a_holder, a_eventName)) {
			return false;
		}

		return _PreviousNotify(a_holder, a_eventName);
	}

	bool StaggerHook::PlayerEx::Hook_NotifyAnimationGraph(
		RE::IAnimationGraphManagerHolder* a_holder,
		const RE::BSFixedString& a_eventName)
	{
		if (!_PreviousNotify) {
			SKSE::log::error("PlayerEx: _PreviousNotify is null");
			return false;
		}

		if (ProcessStaggerEvent(a_holder, a_eventName)) {
			return false;
		}

		return _PreviousNotify(a_holder, a_eventName);
	}

	bool StaggerHook::ProcessStaggerEvent(
		RE::IAnimationGraphManagerHolder* a_holder,
		const RE::BSFixedString& a_eventName)
	{
		// Todo evento de todo ator passa aqui: filtra pelo nome antes de qualquer cast/lock
		if (!(a_eventName == EventStrings::staggerStart) && !(a_eventName == EventStrings::recoilStart)) {
			return false;
		}

		auto actor = a_holder ? skyrim_cast<RE::Actor*>(a_holder) : nullptr;
		if (!actor) {
			return false;
		}

		auto mgr = StaggerManager::GetSingleton();

		// Debounce: evento duplicado de um bloqueio continua bloqueado
		const float DEBOUNCE_MS = 80.0f;
		if (mgr->IsEventTooSoon(actor, DEBOUNCE_MS)) {
			return mgr->WasBlockedRecently(actor, DEBOUNCE_MS);
		}

		float magnitude = 0.0f;
		actor->GetGraphVariableFloat("staggerMagnitude", magnitude);
		float maxMag = mgr->GetMaxMagnitudeInWindow(actor, magnitude, 1.0f);

		auto source = mgr->DetectStaggerSource(actor);

		SKSE::log::debug("Stagger[{}]: {} (Player={}, mag={:.2f}, source={})",
			a_eventName.c_str(), actor->GetName(), actor->IsPlayerRef(), maxMag,
			source == StaggerSource::Magic ? "MAG" :
			source == StaggerSource::Projectile ? "PROJ" : "PHY");

		// Sem hit recente = shout/física: deixa passar
		if (source == StaggerSource::Unknown) {
			return false;
		}

		return mgr->CheckAndProcessStagger(actor, maxMag, source).result == StaggerResult::Block;
	}

	// ============================================================
	//  UPDATE HOOK
	// ============================================================

	void UpdateHook::InstallHook()
	{
		PlayerEx::InstallHook();
	}

	void UpdateHook::PlayerEx::InstallHook()
	{
		REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_PlayerCharacter[0] };

		auto previousAddr = HookProtection::SafeInstallHook(
			vtbl,
			0x0AD,
			reinterpret_cast<std::uintptr_t>(&Hook_Update),
			"VTABLE_PlayerCharacter[0]::Update");

		_PreviousUpdate = reinterpret_cast<UpdateFn>(previousAddr);

		if (!_PreviousUpdate) {
			SKSE::log::error("Failed to get previous Update function");
		}
	}

	void UpdateHook::PlayerEx::Hook_Update(RE::PlayerCharacter* a_this, float a_delta)
	{
		if (!_PreviousUpdate) {
			SKSE::log::error("PlayerEx::Update: _PreviousUpdate is null");
			return;
		}

		_PreviousUpdate(a_this, a_delta);

		cleanupTimer += a_delta;

		if (cleanupTimer >= CLEANUP_INTERVAL) {
			cleanupTimer = 0.0f;
			StaggerManager::GetSingleton()->CleanupDeadActors();
		}
	}
}
