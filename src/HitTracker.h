// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 ByOcire

#pragma once
#include "StaggerManager.h"

namespace AntiStaggerLock
{
	// Registra hits físicos (melee/projétil).
	// Os sinks rodam na ordem de registro; outro mod pode consumir o evento antes.
	class HitEventHandler : public RE::BSTEventSink<RE::TESHitEvent>
	{
	public:
		static HitEventHandler* GetSingleton()
		{
			static HitEventHandler singleton;
			return &singleton;
		}

		static void Register()
		{
			auto* eventSource = RE::ScriptEventSourceHolder::GetSingleton();
			if (!eventSource) {
				SKSE::log::error("Failed to get ScriptEventSourceHolder");
				return;
			}

			eventSource->AddEventSink<RE::TESHitEvent>(GetSingleton());
			SKSE::log::info("Physical hit tracking registered");
		}

		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESHitEvent* a_event,
			RE::BSTEventSource<RE::TESHitEvent>*) override
		{
			if (!a_event || !a_event->target) {
				return RE::BSEventNotifyControl::kContinue;
			}

			auto victim = a_event->target->As<RE::Actor>();
			if (!victim) {
				return RE::BSEventNotifyControl::kContinue;
			}

			// Evento sem origem nenhuma: ignorar
			if (a_event->source == 0 && a_event->projectile == 0 && !a_event->cause) {
				return RE::BSEventNotifyControl::kContinue;
			}

			StaggerSource source = StaggerSource::Melee;

			if (a_event->projectile) {
				auto projectile = RE::TESForm::LookupByID<RE::BGSProjectile>(a_event->projectile);
				if (projectile) {
					// Explosão, feixe, cone e lança-chamas contam como magia
					if (projectile->data.flags.any(
							RE::BGSProjectileData::BGSProjectileFlags::kExplosion,
							RE::BGSProjectileData::BGSProjectileFlags::kExplosionAltTrigger) ||
						projectile->data.types.any(
							RE::BGSProjectileData::Type::kBeam,
							RE::BGSProjectileData::Type::kCone,
							RE::BGSProjectileData::Type::kFlamethrower)) {
						source = StaggerSource::Magic;
					} else {
						source = StaggerSource::Projectile;
					}
				} else {
					source = StaggerSource::Projectile;
				}
			}

			RE::FormID attackerID = 0;
			if (a_event->cause) {
				if (auto attacker = a_event->cause->As<RE::Actor>()) {
					attackerID = attacker->GetFormID();
				}
			}

			StaggerManager::GetSingleton()->RegisterHit(victim, attackerID, source);

			// Sempre kContinue: não atrapalhar outros mods
			return RE::BSEventNotifyControl::kContinue;
		}

	private:
		HitEventHandler() = default;
		~HitEventHandler() override = default;
		HitEventHandler(const HitEventHandler&) = delete;
		HitEventHandler& operator=(const HitEventHandler&) = delete;
	};

	// Registra efeitos mágicos ofensivos
	class MagicHitEventHandler : public RE::BSTEventSink<RE::TESMagicEffectApplyEvent>
	{
	public:
		static MagicHitEventHandler* GetSingleton()
		{
			static MagicHitEventHandler singleton;
			return &singleton;
		}

		static void Register()
		{
			auto* eventSource = RE::ScriptEventSourceHolder::GetSingleton();
			if (!eventSource) {
				SKSE::log::error("Failed to get ScriptEventSourceHolder");
				return;
			}

			eventSource->AddEventSink<RE::TESMagicEffectApplyEvent>(GetSingleton());
			SKSE::log::info("Magic hit tracking registered");
		}

		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESMagicEffectApplyEvent* a_event,
			RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*) override
		{
			if (!a_event || !a_event->target) {
				return RE::BSEventNotifyControl::kContinue;
			}

			auto victim = a_event->target->As<RE::Actor>();
			if (!victim || a_event->magicEffect == 0) {
				return RE::BSEventNotifyControl::kContinue;
			}

			auto baseEffect = RE::TESForm::LookupByID<RE::EffectSetting>(a_event->magicEffect);
			if (!baseEffect) {
				return RE::BSEventNotifyControl::kContinue;
			}

			// O evento não traz o MagicItem de origem, então shout e feitiço não
			// se distinguem aqui; o filtro de efeito ofensivo abaixo basta.
			bool isOffensive = false;

			if (baseEffect->data.flags.any(RE::EffectSetting::EffectSettingData::Flag::kDetrimental)) {
				isOffensive = true;
			} else {
				auto archetype = baseEffect->data.archetype;
				if (archetype == RE::EffectArchetypes::ArchetypeID::kValueModifier ||
					archetype == RE::EffectArchetypes::ArchetypeID::kPeakValueModifier) {
					isOffensive = baseEffect->data.primaryAV == RE::ActorValue::kHealth ||
					              baseEffect->data.primaryAV == RE::ActorValue::kStamina;
				}
			}

			if (!isOffensive) {
				return RE::BSEventNotifyControl::kContinue;
			}

			RE::FormID attackerID = 0;
			if (a_event->caster) {
				if (auto attacker = a_event->caster->As<RE::Actor>()) {
					attackerID = attacker->GetFormID();
				}
			}

			StaggerManager::GetSingleton()->RegisterHit(victim, attackerID, StaggerSource::Magic);

			return RE::BSEventNotifyControl::kContinue;
		}

	private:
		MagicHitEventHandler() = default;
		~MagicHitEventHandler() override = default;
		MagicHitEventHandler(const MagicHitEventHandler&) = delete;
		MagicHitEventHandler& operator=(const MagicHitEventHandler&) = delete;
	};
}
