# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 ByOcire

cmake_minimum_required(VERSION 3.24)  # OVERRIDE_FIND_PACKAGE needs 3.24+

project(AntiStaggerLock LANGUAGES CXX VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
#  Vendored dependencies in extern/ (no vcpkg)
# ============================================================
# CommonLibSSE-NG and DKUtil call find_package() for their own dependencies.
# Each one is cloned into extern/ and registered through FetchContent with
# OVERRIDE_FIND_PACKAGE, so those calls resolve to the local sources.
#
# Clone before configuring (pinned versions):
#   git clone --branch v8.4.1 --recurse-submodules --depth 1 https://github.com/alandtse/CommonLibSSE-NG.git extern/CommonLibSSE-NG
#   git clone --depth 1 https://github.com/gottyduke/DKUtil.git extern/DKUtil
#   git clone --branch v1.16.0 --depth 1 https://github.com/gabime/spdlog.git extern/spdlog
#   git clone --branch v3.12.0 --depth 1 https://github.com/nlohmann/json.git extern/json
#   git clone --branch v7.28 --depth 1 https://github.com/herumi/xbyak.git extern/xbyak
#   git clone --branch v9.07 --depth 1 https://github.com/d99kris/rapidcsv.git extern/rapidcsv
#   git clone --branch oct2025 --depth 1 https://github.com/microsoft/DirectXTK.git extern/DirectXTK
#   git clone --branch jun2026 --depth 1 https://github.com/microsoft/DirectXMath.git extern/DirectXMath
#
# License: since v8.x, alandtse/CommonLibSSE-NG is GPL-3.0-or-later (it was
# MIT until August 2026). Linking it statically makes the .dll a combined
# work, so the AntiStaggerLock source must be available under
# GPL-3.0-or-later (or a compatible license) when the binary is distributed.

set(COMMONLIBSSE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibSSE-NG")
set(DKUTIL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/DKUtil")

set(_REQUIRED_EXTERN_DIRS spdlog json xbyak rapidcsv DirectXTK DirectXMath CommonLibSSE-NG DKUtil SimpleIni)
foreach(_dir ${_REQUIRED_EXTERN_DIRS})
	if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/${_dir}/CMakeLists.txt")
		message(FATAL_ERROR "extern/${_dir} not found (or missing CMakeLists.txt). Clone it into extern/ before configuring; see the commands at the top of this file.")
	endif()
endforeach()

include(FetchContent)

set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "" FORCE) # use spdlog's bundled fmt
FetchContent_Declare(spdlog        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/spdlog"       OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(nlohmann_json SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/json"         OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(xbyak         SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/xbyak"        OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(directxmath   SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/DirectXMath"  OVERRIDE_FIND_PACKAGE)

set(BUILD_TOOLS OFF CACHE BOOL "" FORCE) # skip DirectXTK's XWBTool
FetchContent_Declare(directxtk     SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/DirectXTK"    OVERRIDE_FIND_PACKAGE)

FetchContent_MakeAvailable(spdlog nlohmann_json xbyak directxmath directxtk)

# DirectXMath and DirectXTK only export namespaced targets through find_package.
# With add_subdirectory only the plain targets exist, so alias them.
if(NOT TARGET Microsoft::DirectXMath)
	add_library(Microsoft::DirectXMath ALIAS DirectXMath)
endif()
if(NOT TARGET Microsoft::DirectXTK)
	add_library(Microsoft::DirectXTK ALIAS DirectXTK)
endif()

# rapidcsv is header-only and located by find_path inside CommonLibSSE-NG;
# point the cache variable at the vendored copy.
set(RAPIDCSV_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/extern/rapidcsv/src" CACHE PATH "" FORCE)

set(BUILD_TESTS OFF CACHE BOOL "Disable CommonLibSSE-NG tests" FORCE)

# CommonLibSSE-NG and DKUtil call install(EXPORT ...) unconditionally, which
# fails at generate time because their dependencies come from add_subdirectory.
# install() is never used here, so it is disabled while they are added.
function(install)
endfunction()

add_subdirectory("${COMMONLIBSSE_DIR}" CommonLibSSE EXCLUDE_FROM_ALL)
add_subdirectory("${DKUTIL_DIR}" DKUtil EXCLUDE_FROM_ALL)

# Restore install()
macro(install)
	_install(${ARGN})
endmacro()

add_library(${PROJECT_NAME} SHARED
	src/version.h
	src/Settings.h
	src/Settings.cpp
	src/StaggerManager.h
	src/StaggerManager.cpp
	src/HitTracker.h
	src/Hooks.h
	src/Hooks.cpp
	src/LoadGame.h
	src/LoadGame.cpp
	src/main.cpp
)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		src
		"${CMAKE_CURRENT_SOURCE_DIR}/extern/SimpleIni" # DKUtil Logger/Config include <SimpleIni.h> directly
)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		CommonLibSSE::CommonLibSSE
		DKUtil::DKUtil
		# DKUtil calls find_package for these but never links them
		nlohmann_json::nlohmann_json
		spdlog::spdlog
)

target_precompile_headers(${PROJECT_NAME}
	PRIVATE
		src/pch.h
)

target_compile_definitions(${PROJECT_NAME}
	PRIVATE
		NOMINMAX
		WIN32_LEAN_AND_MEAN
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	OUTPUT_NAME "AntiStaggerLock"
)

# Copy the DLL and INI to the local dist folder
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory
		"${CMAKE_BINARY_DIR}/dist/SKSE/Plugins"
	COMMAND ${CMAKE_COMMAND} -E copy
		"$<TARGET_FILE:${PROJECT_NAME}>"
		"${CMAKE_BINARY_DIR}/dist/SKSE/Plugins/"
	COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_SOURCE_DIR}/dist/AntiStaggerLock.ini"
		"${CMAKE_BINARY_DIR}/dist/SKSE/Plugins/"
)
