# Static lib containing nothing but the MIDL-generated glue for Blackmagic's
# DeckLinkAPI.idl (proxy/stub + GUID definitions). bmsdk/ ships no prebuilt
# .lib — this project *is* the "library" other targets link against.

find_program(MIDL_EXECUTABLE midl.exe REQUIRED)

set(DECKLINK_IDL ${CMAKE_CURRENT_SOURCE_DIR}/../bmsdk/include/DeckLinkAPI.idl)
set(DECKLINK_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(DECKLINK_H ${DECKLINK_GEN_DIR}/DeckLinkAPI_h.h)
set(DECKLINK_C ${DECKLINK_GEN_DIR}/DeckLinkAPI_i.c)

file(MAKE_DIRECTORY ${DECKLINK_GEN_DIR})

add_custom_command(
    OUTPUT ${DECKLINK_H} ${DECKLINK_C}
    COMMAND ${MIDL_EXECUTABLE}
        /nologo
        /h DeckLinkAPI_h.h
        /iid DeckLinkAPI_i.c
        /out ${DECKLINK_GEN_DIR}
        /win64
        ${DECKLINK_IDL}
    MAIN_DEPENDENCY ${DECKLINK_IDL}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/../bmsdk/include/DeckLinkAPITypes.idl
        ${CMAKE_CURRENT_SOURCE_DIR}/../bmsdk/include/DeckLinkAPIModes.idl
        ${CMAKE_CURRENT_SOURCE_DIR}/../bmsdk/include/DeckLinkAPIDiscovery.idl
        ${CMAKE_CURRENT_SOURCE_DIR}/../bmsdk/include/DeckLinkAPIConfiguration.idl
        ${CMAKE_CURRENT_SOURCE_DIR}/../bmsdk/include/DeckLinkAPIDeckControl.idl
        ${CMAKE_CURRENT_SOURCE_DIR}/../bmsdk/include/DeckLinkAPIStreaming.idl
    COMMENT "Running MIDL on DeckLinkAPI.idl"
    VERBATIM
)

add_library(DeckLink STATIC ${DECKLINK_C})
target_include_directories(DeckLink PUBLIC ${DECKLINK_GEN_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../bmsdk/include)
target_compile_definitions(DeckLink PRIVATE $<$<CONFIG:Debug>:_DEBUG> $<$<CONFIG:Release>:NDEBUG> _LIB)

ez_target_common(DeckLink)
