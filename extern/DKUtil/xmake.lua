set_project("DKUtil")
set_version("2.0.0")
set_xmakever("2.8.5")

set_languages("c++23")
set_allowedplats("windows")
set_allowedarchs("x64")

add_rules("mode.debug", "mode.release")

add_requires("nlohmann_json", "spdlog", "xbyak", "simpleini")

target("DKUtil")
    set_kind("headeronly")

    add_headerfiles("include/(**.hpp)")
    add_includedirs("include", { public = true })
    add_packages("nlohmann_json", "spdlog", "xbyak", "simpleini", { public = true })

    add_syslinks("ole32", "shell32", "user32", "version", { public = true })

    add_cxxflags("/Zc:preprocessor", "/Zc:__cplusplus", "/permissive-", "/utf-8", { tools = "cl", public = true })
target_end()
