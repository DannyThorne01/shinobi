
cmake_minimum_required(VERSION 3.15)
project(web_server VERSION 1.0 LANGUAGES C)

# set compiler options
set(CMAKE_C_STANDARD 11)        # Use C11 standard
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)     # Enforce standard compliance

# add source files
set(SOURCES
    src/main.c
)

# create the executable
add_executable(web_server ${SOURCES})

# include directories
target_include_directories(web_server PRIVATE src)

# had to add this so networking functions work
# _DEFAULT_SOURCE re-enables BSD/GNU extensions (like SA_RESTART) that
# _POSIX_C_SOURCE alone would hide
add_compile_definitions(_POSIX_C_SOURCE=200112L _DEFAULT_SOURCE)

# optional: enable warnings and debugging flags
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(web_server PRIVATE -Wall -Wextra -Wpedantic -g)
endif()

find_package(GDAL REQUIRED)

target_link_libraries(web_server GDAL::GDAL)

# GDAL needs this - it uses C++ internally even though you call it from C
# so link the C++ standard library too
target_link_libraries(web_server stdc++)