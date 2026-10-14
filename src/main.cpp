// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 ByOcire

#include "LoadGame.h"
#include "version.h"
#include <spdlog/sinks/basic_file_sink.h>

#ifndef DLLEXPORT
#define DLLEXPORT extern "C" [[maybe_unused]] __declspec(dllexport)
#endif

DLLEXPORT auto SKSEPlugin_Version = []() noexcept {
	SKSE::PluginVersionData data{};

	data.PluginVersion(Plugin::VERSION);
	data.PluginName(Plugin::NAME);
	data.AuthorName(Plugin::AUTHOR);
	data.UsesAddressLibrary();
	data.UsesUpdatedStructs();

	return data;
}();

DLLEXPORT bool SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo* pluginInfo)
{
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;

	return true;
}

DLLEXPORT bool SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);

	// Log em Documents\My Games\...\SKSE\AntiStaggerLock.log
	auto path = SKSE::log::log_directory();
	if (!path) {
		return false;
	}

	*path /= std::string(Plugin::NAME) + ".log";
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
	auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S] [%l] %v");

	SKSE::log::info("{} v{} loading", Plugin::NAME, Plugin::VERSION.string());

	SKSE::AllocTrampoline(128);

	auto messaging = SKSE::GetMessagingInterface();
	if (!messaging) {
		SKSE::log::error("Failed to get messaging interface");
		return false;
	}

	if (!messaging->RegisterListener(AntiStaggerLock::EventCallback)) {
		SKSE::log::error("Failed to register messaging listener");
		return false;
	}

	SKSE::log::info("{} loaded", Plugin::NAME);
	return true;
}
