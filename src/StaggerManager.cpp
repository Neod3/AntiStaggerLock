// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 ByOcire

#include "StaggerManager.h"
#include "Settings.h"
#include <algorithm>

namespace AntiStaggerLock
{
	using namespace std::chrono;

	static ActorStaggerData& GetOrCreateData(std::unordered_map<RE::FormID, ActorStaggerData>& map, RE::FormID id)
	{
		return map.try_emplace(id).first->second;
	}

	// Entrada recém-criada (time_point::min()) conta como ociosidade 0;
	// subtrair min() de "agora" estouraria o tipo.
	static long long IdleSeconds(TimePoint now, TimePoint last)
	{
		if (last == TimePoint::min()) {
			return 0;
		}
		return duration_cast<seconds>(now - last).count();
	}

	// ============================================================
	//  HIT TRACKING
	// ============================================================

	void StaggerManager::RegisterHit(RE::Actor* victim, RE::FormID attackerID, StaggerSource source)
	{
		if (!victim) return;

		auto formID = victim->GetFormID();

		std::unique_lock lock(dataMutex);

		HitData& data = recentHits[formID];

		// Efeitos mágicos passivos chegam junto de todo golpe físico;
		// não podem sobrescrever a fonte física registrada logo antes.
		if (source == StaggerSource::Magic &&
			(data.source == StaggerSource::Melee || data.source == StaggerSource::Projectile) &&
			steady_clock::now() - data.timestamp < milliseconds(500)) {
			return;
		}

		data.source = source;
		data.attackerID = attackerID;
		data.timestamp = steady_clock::now();
	}

	std::optional<HitData> StaggerManager::GetRecentHit(RE::Actor* actor, float maxAgeSeconds)
	{
		if (!actor) return std::nullopt;

		std::shared_lock lock(dataMutex);

		auto it = recentHits.find(actor->GetFormID());
		if (it == recentHits.end()) {
			return std::nullopt;
		}

		auto age = duration_cast<milliseconds>(steady_clock::now() - it->second.timestamp).count() / 1000.0f;
		if (age > maxAgeSeconds) {
			return std::nullopt;
		}

		return it->second;
	}

	StaggerSource StaggerManager::DetectStaggerSource(RE::Actor* a_actor)
	{
		if (!a_actor) return StaggerSource::Melee;

		if (auto hit = GetRecentHit(a_actor, 0.5f)) {
			return hit->source;
		}

		return StaggerSource::Unknown;
	}

	// ============================================================
	//  STAGGER CONTROL
	// ============================================================

	StaggerDecision StaggerManager::CheckAndProcessStagger(RE::Actor* a_actor, float a_magnitude, StaggerSource a_source)
	{
		StaggerDecision decision;

		if (!a_actor) {
			return decision;
		}

		auto settings = Settings::GetSingleton();
		if (!*settings->EnableAntiStagger || !ShouldApplyAntiStagger(a_actor)) {
			return decision;
		}

		// Sem hit conhecido (shout/física): sempre passa, sem cooldown
		if (a_source == StaggerSource::Unknown) {
			return decision;
		}

		const auto now = steady_clock::now();
		const bool isMagic = (a_source == StaggerSource::Magic);

		// Verificação + atualização sob o mesmo lock (evita race entre threads)
		std::unique_lock lock(dataMutex);

		auto& data = GetOrCreateData(actorData, a_actor->GetFormID());
		auto& lastTime = isMagic ? data.lastMagicStaggerTime : data.lastMeleeStaggerTime;

		const float cooldown = static_cast<float>(isMagic ? *settings->MagicStaggerCooldown : *settings->StaggerCooldownTime);

		float timeSince = 999.0f;
		if (lastTime != TimePoint::min()) {
			timeSince = duration_cast<milliseconds>(now - lastTime).count() / 1000.0f;
		}

		// Golpe físico forte ignora o cooldown (mas inicia um novo). Magia nunca.
		const bool isStrongAttack = !isMagic && a_magnitude > static_cast<float>(*settings->MaxMagnitudeToBlock);

		data.lastProcessedEventTime = now;

		if (timeSince < cooldown && !isStrongAttack) {
			SKSE::log::info("Stagger BLOCKED: {} (mag {:.2f}, {}, {:.2f}s < {:.2f}s)",
				a_actor->GetName(), a_magnitude, isMagic ? "magic" : "physical", timeSince, cooldown);

			data.lastBlockedTime = now;
			decision.result = StaggerResult::Block;

			if (auto st = a_actor->AsActorState()) {
				st->actorState2.staggered = 0;
			}

			return decision;
		}

		SKSE::log::info("Stagger ALLOWED{}: {} (mag {:.2f}, {}) - cooldown started",
			isStrongAttack ? " (strong)" : "", a_actor->GetName(), a_magnitude, isMagic ? "magic" : "physical");

		lastTime = now;
		decision.result = StaggerResult::AllowWithCD;
		return decision;
	}

	// ============================================================
	//  UTILITIES
	// ============================================================

	bool StaggerManager::ShouldApplyAntiStagger(RE::Actor* a_actor)
	{
		if (!a_actor) return false;

		auto settings = Settings::GetSingleton();

		if (a_actor->IsPlayerRef()) {
			return *settings->ApplyToPlayer;
		}

		auto actorBase = a_actor->GetActorBase();
		if (!actorBase) return false;

		auto race = actorBase->GetRace();
		if (!race) return false;

		// kPlayable = humanoide (NPC); o resto é criatura
		bool isPlayable = race->data.flags.any(RE::RACE_DATA::Flag::kPlayable);
		return isPlayable ? *settings->ApplyToNPCs : *settings->ApplyToCreatures;
	}

	void StaggerManager::CleanupDeadActors()
	{
		std::unique_lock lock(dataMutex);

		if (actorData.size() > MAX_CACHE_SIZE) {
			SKSE::log::warn("Cache above limit ({} entries), cleaning up", actorData.size());
		}

		// Remove atores mortos, deletados ou inativos há mais de 5 min
		std::vector<RE::FormID> toRemove;
		auto now = steady_clock::now();

		for (auto& [formID, data] : actorData) {
			auto actor = RE::TESForm::LookupByID<RE::Actor>(formID);
			if (!actor || actor->IsDead() || actor->IsDeleted() ||
				IdleSeconds(now, data.lastProcessedEventTime) > 300) {
				toRemove.push_back(formID);
			}
		}

		for (auto formID : toRemove) {
			actorData.erase(formID);
			recentHits.erase(formID);
		}

		if (!toRemove.empty()) {
			SKSE::log::debug("Cleaned {} actors from cache", toRemove.size());
		}
	}

	bool StaggerManager::IsEventTooSoon(RE::Actor* a_actor, float a_windowMs)
	{
		if (!a_actor) return true;

		auto now = steady_clock::now();

		std::unique_lock lock(dataMutex);
		auto& data = GetOrCreateData(actorData, a_actor->GetFormID());

		if (data.lastProcessedEventTime == TimePoint::min()) {
			return false;
		}

		auto delta = duration_cast<milliseconds>(now - data.lastProcessedEventTime).count();
		return delta < static_cast<long long>(a_windowMs);
	}

	bool StaggerManager::WasBlockedRecently(RE::Actor* a_actor, float a_windowMs)
	{
		if (!a_actor) return false;

		std::shared_lock lock(dataMutex);

		auto it = actorData.find(a_actor->GetFormID());
		if (it == actorData.end() || it->second.lastBlockedTime == TimePoint::min()) {
			return false;
		}

		auto delta = duration_cast<milliseconds>(steady_clock::now() - it->second.lastBlockedTime).count();
		return delta < static_cast<long long>(a_windowMs);
	}

	// Maior magnitude vista na janela (o motor pode reportar vários valores por hit)
	float StaggerManager::GetMaxMagnitudeInWindow(RE::Actor* a_actor, float incomingMagnitude, float windowSeconds)
	{
		if (!a_actor) return incomingMagnitude;

		auto now = steady_clock::now();

		std::unique_lock lock(dataMutex);
		auto& data = GetOrCreateData(actorData, a_actor->GetFormID());

		if (data.lastMagnitudeUpdate != TimePoint::min()) {
			auto delta = duration_cast<milliseconds>(now - data.lastMagnitudeUpdate).count() / 1000.0f;

			if (delta < windowSeconds) {
				float maxValue = std::max(incomingMagnitude, data.lastRecordedMagnitude);
				data.lastRecordedMagnitude = maxValue;
				return maxValue;
			}
		}

		data.lastRecordedMagnitude = incomingMagnitude;
		data.lastMagnitudeUpdate = now;
		return incomingMagnitude;
	}
}
