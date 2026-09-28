cmake_minimum_required(VERSION 3.21)

project(PPLTools VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PPL_STATIC_RUNTIME "Link the C++ runtime statically (single ppl.exe without DLLs)" ON)
option(PPL_BUILD_TESTS "Build unit tests" ON)

if(MINGW AND PPL_STATIC_RUNTIME)
    add_link_options(-static -static-libgcc -static-libstdc++)
endif()
if(MINGW)
    add_link_options($<$<CONFIG:Release>:-s>)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
elseif(MSVC)
    add_compile_options(/W4 /utf-8)
endif()

# Embed the command database into the executable.
set(PPL_COMMANDS_FILE ${CMAKE_CURRENT_SOURCE_DIR}/data/commands.json)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${PPL_COMMANDS_FILE})
file(READ ${PPL_COMMANDS_FILE} PPL_COMMANDS_JSON)
configure_file(core/commanddata.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/generated/commanddata.cpp @ONLY)

add_library(pplcore STATIC
    core/utf8.h core/utf8.cpp
    core/json.h core/json.cpp
    core/diagnostic.h core/token.h
    core/lexer.h core/lexer.cpp
    core/commanddb.h core/commanddb.cpp
    core/analyzer.h core/analyzer.cpp
    core/formatter.h core/formatter.cpp
    core/services.h core/services.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated/commanddata.cpp
)
target_include_directories(pplcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core)

add_executable(ppl
    ppl/main.cpp
    ppl/cli.h ppl/cli.cpp
    ppl/jsonrpc.h ppl/jsonrpc.cpp
    ppl/lsp.h ppl/lsp.cpp
    ppl/mcp.h ppl/mcp.cpp
    ppl/generators.h ppl/generators.cpp
)
target_link_libraries(ppl PRIVATE pplcore)
target_compile_definitions(ppl PRIVATE PPL_VERSION="${PROJECT_VERSION}")

install(TARGETS ppl RUNTIME DESTINATION bin)

if(PPL_BUILD_TESTS)
    enable_testing()
    add_executable(ppltests tests/tests.cpp tests/testing.h)
    target_link_libraries(ppltests PRIVATE pplcore)
    target_compile_definitions(ppltests PRIVATE PPL_REPO_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/..")
    add_test(NAME ppltests COMMAND ppltests)
endif()
