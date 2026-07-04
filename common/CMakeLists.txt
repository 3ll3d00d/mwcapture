find_package(quill CONFIG REQUIRED)

add_library(common STATIC
    audio_capture_pin.cpp
    capture_filter.cpp
    capture_pin.cpp
    modeswitcher.cpp
    signalinfo.cpp
    video_capture_pin.cpp
)

target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(common PUBLIC directshow_baseclasses PRIVATE quill::quill)

target_compile_definitions(common PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _LIB
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    # Release/NoArch drop Quill (NO_QUILL); Trace builds keep it and tag the
    # binary name instead (CAPTURE_NAME_SUFFIX) -- see EZ_ENABLE_TRACE.
    $<$<AND:$<CONFIG:Release>,$<NOT:$<BOOL:${EZ_ENABLE_TRACE}>>>:NO_QUILL>
    $<$<AND:$<CONFIG:Release>,$<BOOL:${EZ_ENABLE_TRACE}>>:CAPTURE_NAME_SUFFIX=1>
    $<$<AND:$<CONFIG:Release>,$<BOOL:${EZ_VERSION_MAJOR}>>:EZ_VERSION_MAJOR=${EZ_VERSION_MAJOR}>
    $<$<AND:$<CONFIG:Release>,$<BOOL:${EZ_VERSION_MINOR}>>:EZ_VERSION_MINOR=${EZ_VERSION_MINOR}>
    $<$<AND:$<CONFIG:Release>,$<BOOL:${EZ_VERSION_PATCH}>>:EZ_VERSION_PATCH=${EZ_VERSION_PATCH}>
    $<$<AND:$<CONFIG:Release>,$<BOOL:${EZ_VERSION_SUFFIX}>>:EZ_VERSION_SUFFIX=${EZ_VERSION_SUFFIX}>
)

ez_target_common(common)
