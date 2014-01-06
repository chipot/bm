# Change this if you need to target a specific CMake version
# ----------------------------------------------------------
cmake_minimum_required(VERSION 2.6)
project(binary_match)

# Definitions for main binary
# ---------------------------
include_directories(${PROJECT_SOURCE_DIR})

# Define sources
# --------------
set(PROJECT_HEADERS
	src/bin.hh)

set(PROJECT_SOURCES
	${BIN_HEADERS})

# Unit testing
# ------------
find_package(Boost COMPONENTS unit_test_framework)

include_directories(src)
add_library(binary-test SHARED
	    ${PROJECT_SOURCES}
	    test/test.cc)
#add_unit_test_library(binary-test)
