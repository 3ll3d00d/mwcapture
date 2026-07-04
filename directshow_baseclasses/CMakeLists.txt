add_library(directshow_baseclasses STATIC
    amextra.cpp
    amfilter.cpp
    amvideo.cpp
    arithutil.cpp
    combase.cpp
    cprop.cpp
    ctlutil.cpp
    ddmm.cpp
    dllentry.cpp
    dllsetup.cpp
    mtype.cpp
    outputq.cpp
    perflog.cpp
    pstream.cpp
    pullpin.cpp
    refclock.cpp
    renbase.cpp
    schedule.cpp
    seekpt.cpp
    source.cpp
    strmctl.cpp
    sysclock.cpp
    transfrm.cpp
    transip.cpp
    videoctl.cpp
    vtrans.cpp
    winctrl.cpp
    winutil.cpp
    wxdebug.cpp
    wxlist.cpp
    wxutil.cpp
)

target_include_directories(directshow_baseclasses PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(directshow_baseclasses PRIVATE $<$<CONFIG:Debug>:_DEBUG> $<$<CONFIG:Release>:NDEBUG> _LIB)

ez_target_common_legacy(directshow_baseclasses)
