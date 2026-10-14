package("dkutil")
    set_kind("library", { headeronly = true })
    set_homepage("https://github.com/gottyduke/DKUtil")
    set_description("An utilitarian library to help with SKSE plugin development.")
    set_license("MIT")

    add_urls("https://github.com/gottyduke/DKUtil.git")
    add_versions("2.0.0", "9098b2371122357aacd6da639f460a2e7e85c7ed")

    add_deps("nlohmann_json", "spdlog", "xbyak", "simpleini")

    add_syslinks("ole32", "shell32", "user32", "version")

    -- /utf-8 by the fmt bundled inside spdlog
    add_cxxflags("/Zc:preprocessor", "/Zc:__cplusplus", "/permissive-", "/utf-8", { tools = "cl" })

    on_install("windows|x64", function (package)
        os.cp("include", package:installdir())
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({ test = [[
            #define WIN32_LEAN_AND_MEAN
            #include <Windows.h>
            #include <string_view>
            using namespace std::literals;
            namespace Plugin { inline constexpr auto NAME = "dkutil-test"sv; }

            #include "DKUtil/Utility.hpp"

            void test() {
                const auto parts = dku::string::split("dku;xmake"sv, ";"sv);
                (void)dku::string::join(parts, "-"sv);
            }
        ]] }, { configs = { languages = "c++23", cxflags = "/Zc:preprocessor /Zc:__cplusplus /utf-8" } }))
    end)
