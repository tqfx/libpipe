target("test/run")
    set_group("test")
    set_default(false)
    set_kind("binary")
    add_files("run.c")
    add_deps("pipe")
target_end()

target("test/sum")
    set_group("test")
    set_default(false)
    set_kind("binary")
    add_files("sum.c")
target_end()