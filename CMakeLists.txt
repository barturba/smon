cmake_minimum_required(VERSION 3.16)
project(smon VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# macOS specific settings
if(APPLE)
    # Target ARM64 architecture
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")
endif()

# Find ncurses
find_package(Curses REQUIRED)
if(NOT CURSES_FOUND)
    message(FATAL_ERROR "ncurses not found. Please install ncurses (brew install ncurses)")
endif()

# Find CoreFoundation (for macOS)
if(APPLE)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
endif()

# Add executable
add_executable(smon main.c)

# Link libraries
target_link_libraries(smon PRIVATE ${CURSES_LIBRARIES})
if(APPLE)
    target_link_libraries(smon PRIVATE ${COREFOUNDATION_LIBRARY})
endif()

# Include directories
target_include_directories(smon PRIVATE ${CURSES_INCLUDE_DIRS})

# Compiler flags
if(APPLE)
    target_compile_options(smon PRIVATE -Wall -Wextra -pedantic)
else()
    target_compile_options(smon PRIVATE -Wall -Wextra -pedantic)
endif()

# Installation
install(TARGETS smon
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
