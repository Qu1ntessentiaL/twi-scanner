cmake_minimum_required(VERSION 3.15)

project(twi-scanner VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TWI_MOCK_FT4222 "Build without LibFT4222 (stub backend, no hardware)" OFF)
option(TWI_REQUIRE_FT4222 "Fail if LibFT4222 is not found" OFF)
option(BUILD_TESTING "Build unit tests" ON)

find_path(FT4222_INCLUDE_DIR NAMES libft4222.h ftd2xx.h
        PATHS /usr/local/include /usr/include)
find_library(FT4222_LIBRARY NAMES ft4222 PATHS /usr/local/lib /usr/lib)

if (TWI_MOCK_FT4222)
    set(TWI_USE_MOCK_FT4222 TRUE)
elseif (FT4222_INCLUDE_DIR AND FT4222_LIBRARY)
    set(TWI_USE_MOCK_FT4222 FALSE)
elseif (TWI_REQUIRE_FT4222)
    message(FATAL_ERROR "libft4222 not found. Install FT4222 Linux library (see README) "
            "or configure with -DTWI_MOCK_FT4222=ON for stub build.")
else()
    message(STATUS "LibFT4222 not found — using mock backend (no hardware I/O).")
    set(TWI_USE_MOCK_FT4222 TRUE)
endif()

function(twi_add_ft4222_backend target)
    if (TWI_USE_MOCK_FT4222)
        target_sources(${target} PRIVATE src/ft4222/ft4222_mock.cpp)
        target_compile_definitions(${target} PRIVATE TWI_MOCK_FT4222=1)
    else()
        target_sources(${target} PRIVATE src/ft4222/ft4222.cpp)
        target_include_directories(${target} PRIVATE ${FT4222_INCLUDE_DIR})
        target_link_libraries(${target} PRIVATE ${FT4222_LIBRARY})
    endif()
endfunction()

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/cli/Cli.cpp
        src/cli/CommandRouter.cpp
        src/cli/Commands.cpp
        src/cli/ParseUtil.cpp
)
twi_add_ft4222_backend(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE src)

find_path(READLINE_INCLUDE_DIR readline/readline.h)
find_library(READLINE_LIBRARY readline)
if (READLINE_INCLUDE_DIR AND READLINE_LIBRARY)
    message(STATUS "Found readline: ${READLINE_LIBRARY}")
    target_include_directories(${PROJECT_NAME} PRIVATE ${READLINE_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${READLINE_LIBRARY})
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_READLINE)
else()
    message(STATUS "readline not found; CLI history/navigation disabled")
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
        TWI_SCANNER_VERSION="${PROJECT_VERSION}")

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (BUILD_TESTING)
    enable_testing()

    add_executable(twi-scanner-test-parse tests/test_parse.cpp src/cli/ParseUtil.cpp)
    target_include_directories(twi-scanner-test-parse PRIVATE src)
    add_test(NAME test-parse COMMAND twi-scanner-test-parse)

    add_executable(twi-scanner-test-router
            tests/test_router.cpp
            src/cli/CommandRouter.cpp
    )
    target_include_directories(twi-scanner-test-router PRIVATE src)
    twi_add_ft4222_backend(twi-scanner-test-router)
    add_test(NAME test-router COMMAND twi-scanner-test-router)
endif()
