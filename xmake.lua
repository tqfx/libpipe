-- set project name
set_project("pipe")

-- set xmake minimum version
set_xmakever("2.5.0")

-- set project version
set_version("0.1.0", {build = "%Y%m%d%H%M"})

-- set language: c11
set_languages("c11")

-- option: warnings
option("warnings")
    set_default(false)
    set_showmenu(true)
    set_description("Enable or disable warnings")
option_end()

if has_config("warnings") then
    -- set warning everything
    set_warnings("everything")
    -- disable some compiler errors
    if is_plat("windows") then
        add_cxflags("/wd4514", "/wd4710", "/wd4711", "/wd5039", "/wd5045")
    end
    add_cxflags("-Wno-reserved-identifier", "-Wno-used-but-marked-unused")
    add_cflags("-Wno-declaration-after-statement")
    add_cxxflags("-Wno-c++98-compat-pedantic")
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
        add_defines("PIPE_IMPORTS", {interface = true})
    end
    -- add include directories
    add_includedirs("include", {public = true})
    -- add the header files for installing
    add_headerfiles("include/(**.h)")
    -- add the common source files
    add_files("src/**.c")
    -- add the platform options
    if not is_plat("windows") then
        add_cxflags("-fPIC")
    end
target_end()

-- include test sources
includes("test")
