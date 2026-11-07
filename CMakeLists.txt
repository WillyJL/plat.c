cmake_minimum_required(VERSION 3.10)

project(plat C)

# Global compiler options
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
include_directories(
    # Source code
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)
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

# Current build symlink
add_custom_target(
    build-current
    COMMAND "${CMAKE_COMMAND}" -E create_symlink "${CMAKE_BINARY_DIR}" "${CMAKE_SOURCE_DIR}/build/current"
)
add_dependencies(${PROJECT_NAME} build-current)

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
