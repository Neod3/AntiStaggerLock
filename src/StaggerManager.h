// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 ByOcire

#pragma once
#include "DKUtil/Utility.hpp"
#include <unordered_map>
#include <chrono>
#include <shared_mutex>
#include <optional>

namespace AntiStaggerLock
{
	using TimePoint = std::chrono::steady_clock::time_point;

	enum class StaggerSource
	{
		Unknown,
		Melee,
		Magic,
		Projectile
	};

	enum class StaggerResult
	{
		Block,
		AllowNoCD,
		AllowWithCD
	};

	struct StaggerDecision
	{
		StaggerResult result = StaggerResult::AllowNoCD;
	};

	// Último hit registrado na vítima
	struct HitData
	{
		StaggerSource source = StaggerSource::Unknown;
		RE::FormID attackerID = 0;
		TimePoint timestamp = TimePoint::min();
	};

	// Estado de cooldown por ator (time_point::min() = nunca ocorreu)
	struct ActorStaggerData
	{
		TimePoint lastMagicStaggerTime = TimePoint::min();
		TimePoint lastMeleeStaggerTime = TimePoint::min();
		TimePoint lastProcessedEventTime = TimePoint::min();
		TimePoint lastBlockedTime = TimePoint::min();
		TimePoint lastMagnitudeUpdate = TimePoint::min();
		float lastRecordedMagnitude = 0.0f;
	};

	class StaggerManager : public DKUtil::model::Singleton<StaggerManager>
	{
		friend DKUtil::model::Singleton<StaggerManager>;

	public:
		StaggerManager() = default;

		void RegisterHit(RE::Actor* victim, RE::FormID attackerID, StaggerSource source);
		std::optional<HitData> GetRecentHit(RE::Actor* actor, float maxAgeSeconds = 0.5f);

		// Checa o cooldown e atualiza o estado sob um único lock
		StaggerDecision CheckAndProcessStagger(RE::Actor* a_actor, float a_magnitude, StaggerSource a_source);

		StaggerSource DetectStaggerSource(RE::Actor* a_actor);

		void CleanupDeadActors();
		bool ShouldApplyAntiStagger(RE::Actor* a_actor);
		bool IsEventTooSoon(RE::Actor* a_actor, float a_windowMs = 100.0f);
		bool WasBlockedRecently(RE::Actor* a_actor, float a_windowMs);
		float GetMaxMagnitudeInWindow(RE::Actor* a_actor, float incomingMagnitude, float windowSeconds = 1.0f);

	private:
		mutable std::shared_mutex dataMutex;

		std::unordered_map<RE::FormID, HitData> recentHits;
		std::unordered_map<RE::FormID, ActorStaggerData> actorData;

		static constexpr size_t MAX_CACHE_SIZE = 500;
	};
}
