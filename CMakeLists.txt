cmake_minimum_required(VERSION 3.15)
project(SimpleTrieSpellChecker C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Specify source and include directories
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

# Add include directories
include_directories(${INCLUDE_DIR})  # Ensures include/ headers are found

# Collect all source files
file(GLOB SOURCES "${SRC_DIR}/*.c")

# Create executable
add_executable(SimpleTrieSpellChecker ${SOURCES})

# Set MSVC specific compiler flags
if (MSVC)
    target_compile_options(SimpleTrieSpellChecker PRIVATE /W4 /WX)
    target_compile_definitions(SimpleTrieSpellChecker PRIVATE _CRT_SECURE_NO_WARNINGS)
    # Increase default stack size (reserve 2 MB)
    set(CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} /STACK:2000000")
endif()

# Set output directory
set_target_properties(SimpleTrieSpellChecker PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
