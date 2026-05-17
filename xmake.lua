add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate")
set_policy("build.ccache", true)

add_requires("cli11", {system = true})

target("dotty")
    add_files("src/*.cpp")
    add_includedirs("src")
    set_languages("c++23")
    add_packages("cli11")
    set_pcheader("src/common.h")
