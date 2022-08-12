-- add the platform options
if is_plat("windows") then
    add_defines("_CRT_SECURE_NO_WARNINGS")
elseif not is_plat("mingw") then
    add_defines("_GNU_SOURCE=1")
end

target("sum")
    set_group("test")
    set_default(false)
    set_kind("binary")
    add_files("sum.c")
target_end()

target("test")
    set_group("test")
    set_default(false)
    set_kind("binary")
    add_files("test.c")
    add_deps("pipe")
target_end()
