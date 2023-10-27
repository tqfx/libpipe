-- set project name
set_project("pipe")

-- set xmake minimum version
set_xmakever("2.5.0")

-- set project version
set_version("0.1.1", { build = "%Y%m%d%H%M" })

-- set language: c99
set_languages("c99")

-- option: warning
option("warning")
set_default(false)
set_showmenu(true)
set_description("Enable or disable warnings")
option_end()

if has_config("warning") then
    -- set warning everything
    set_warnings("everything")
    -- disable some compiler errors
    if is_plat("windows") then
        add_cxflags("/wd4514", "/wd4710", "/wd4711", "/wd5039", "/wd5045")
    end
    add_cflags("-Wno-declaration-after-statement")
    add_cxxflags("-Wno-c++98-compat-pedantic")
    add_cxflags("-Wno-reserved-identifier")
end

-- add build modes
add_rules("mode.check", "mode.debug", "mode.release")
if is_mode("check") and not is_plat("mingw") then
    local flags = {
        "-fsanitize=address,undefined",
        "-fsanitize-recover=address",
        "-fno-omit-frame-pointer",
        "-fsanitize=leak",
    }
    add_cxflags(flags)
    add_ldflags(flags)
end

target("pipe")
-- make as a static/shared library
set_kind("$(kind)")
-- export symbols
add_defines("PIPE_EXPORTS")
-- set shared library symbols
if is_kind("shared") then
    -- import symbols
    add_defines("PIPE_IMPORTS", { interface = true })
end
-- add include directories
add_includedirs("include", { public = true })
-- add the header files for installing
add_headerfiles("include/(**.h)")
-- add the common source files
add_files("src/**.c")
-- add the platform options
if not is_plat("windows") then
    add_cxflags("-fPIC")
end
target_end()

-- include module sources
if not table.empty(os.files("*/xmake.lua")) then
    includes("*/xmake.lua")
end
