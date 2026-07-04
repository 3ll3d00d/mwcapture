add_executable(benchtest bench.cpp)

target_compile_definitions(benchtest PRIVATE
    _CONSOLE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

ez_target_common(benchtest)
