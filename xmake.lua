set_arch("x86_64")
set_plat("linux")
set_toolchains("gcc")

add_rules("mode.debug", "mode.release")
set_defaultmode("debug")

add_rules("plugin.compile_commands.autoupdate")
set_policy("build.ccache", false)
-- add_rules("c++.unity_build")

add_requires("cli11", {system = true})
add_requires("readline", {system = true})



if is_mode("release") then
    add_defines("DEBUG_ON=0")
    set_strip("all")
end
--
if is_mode("debug") then
    set_optimize("none")
end
---
set_languages("c++23")
add_cxxflags("-Wno-ignored-attributes")
set_pcxxheader("include/common.hpp")
set_symbols("none")

target("core")
    set_kind("static")

    add_files("core/src/*.cpp")
    add_includedirs("include/", "core/include/")


target("dotty")
    set_kind("binary")

    add_files("src/*.cpp")
    add_includedirs("include/", "core/include/")

    add_deps("core")
    add_packages("cli11", "readline")
