cmake_minimum_required(VERSION 3.10)

project(plat C)

# Global compiler options
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
add_compile_options(
    # Show colors through Ninja pipes
    "-fdiagnostics-color=always"
    # Stop on first error
    "-Wfatal-errors"
    # Relative path for __FILE__
    "-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=."
)

# Main library
file(GLOB sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
add_library(${PROJECT_NAME} "${sources}")
target_compile_options(${PROJECT_NAME} PRIVATE "-Wall" "-Wextra" "-Werror")
target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-error=unused-function" "-Wno-error=unused-label" "-Wno-error=unused-parameter" "-Wno-error=unused-variable" "-Wno-error=unused-but-set-variable")
target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-error=unused-result")
target_include_directories(
    ${PROJECT_NAME} PUBLIC
    # Source code
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)

# OS specific configuration
if(WIN32)
    # Target Windows 10 APIs, keep windows.h lean and without min/max macros
    target_compile_definitions(${PROJECT_NAME} PRIVATE "_WIN32_WINNT=0x0A00" "WINVER=0x0A00" "WIN32_LEAN_AND_MEAN" "NOMINMAX")
    # BCryptGenRandom()
    target_link_libraries(${PROJECT_NAME} PUBLIC bcrypt)
endif()

# Clangd
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/.clangd.in ${CMAKE_CURRENT_SOURCE_DIR}/.clangd @ONLY)

if(PROJECT_IS_TOP_LEVEL)
    # Current build symlink
    add_custom_target(
        build-current
        COMMAND "${CMAKE_COMMAND}" -E create_symlink "${CMAKE_BINARY_DIR}" "${CMAKE_SOURCE_DIR}/build/current"
    )
    add_dependencies(${PROJECT_NAME} build-current)
endif()

# M*LIB
target_compile_options(
    ${PROJECT_NAME} PUBLIC
    "-DM_USE_SMALL_NAME=0"  # Disable legacy type/function names
)

# Format sources
add_custom_target(
    plat-format
    COMMAND clang-format -i -- ${sources}
)
# Check that formatting is correct
add_custom_target(
    plat-lint
    COMMAND clang-format -n -Werror -- ${sources}
)
