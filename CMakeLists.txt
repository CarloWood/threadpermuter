cmake_minimum_required(VERSION 3.14...3.23)

# Compile all subdirectories with the same standard.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(threadpermuter
  LANGUAGES C CXX
  DESCRIPTION "Testsuite for threaded snippets."
)

# Begin of gitache configuration.
set(GITACHE_PACKAGES libcwd_r)

include(FetchContent)

# If a local gitache submodule is present then use that rather than downloading one.
if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/gitache/.git)
  # This will disable the use of the GIT_REPOSITORY/GIT_TAG below, and disable the
  # FetchContent- download and update step. Instead, use the gitache submodule as-is.
  set(FETCHCONTENT_SOURCE_DIR_GITACHE "${CMAKE_CURRENT_LIST_DIR}/gitache" CACHE INTERNAL "" FORCE)
endif ()

FetchContent_Declare(
  gitache
  GIT_REPOSITORY "https://github.com/CarloWood/gitache.git"
  GIT_TAG        "stable"       # Normally use either "stable" or a fixed SHA1 to freeze the version here!
                                # Using "master" might be *unstable*.
)
FetchContent_MakeAvailable(gitache)
# End of gitache configuration.

 # This project uses aicxx modules.
include(cwm4/cmake/AICxxProject)

# Exit if someone tries to contaminate the source directory with an in-source build.
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "Please do out-of-source builds."
                      "Cleanup: \"rm -rf CMake CMakeCache.txt CMakeFiles/\"")
endif ()

include(AICxxSubmodules)

add_library(threadpermuter_ObjLib OBJECT)

# Set SOURCE_DIR to the current directory.
set_property(
  TARGET threadpermuter_ObjLib
  PROPERTY SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
)

# The list of source files.
target_sources(threadpermuter_ObjLib
  PRIVATE
    ThreadPermuter.cxx Permutation.cxx Thread.cxx
    ThreadPermuter.h Permutation.h Thread.h
)

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(threadpermuter_ObjLib
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
)

# Set link dependencies.
target_link_libraries(threadpermuter_ObjLib
  PUBLIC
    ${libcwd_r_TARGET}
    Threads::Threads
)

# Create an ALIAS target.
add_library(ThreadPermuter::threadpermuter ALIAS threadpermuter_ObjLib)

# Test executable.
add_executable(bstest bstest.cxx)
target_link_libraries(bstest ${AICXX_OBJECTS_LIST})

add_executable(permute_test permute_test.cxx)
target_link_libraries(permute_test ThreadPermuter::threadpermuter ${AICXX_OBJECTS_LIST})

add_executable(FuzzyLock_test FuzzyLock_test.cxx)
target_link_libraries(FuzzyLock_test ThreadPermuter::threadpermuter ${AICXX_OBJECTS_LIST})

add_executable(StateChanger_test StateChanger_test.cxx)
target_link_libraries(StateChanger_test ThreadPermuter::threadpermuter ${AICXX_OBJECTS_LIST})

add_executable(StreamBufReset_test StreamBufReset_test.cxx)
target_link_libraries(StreamBufReset_test ThreadPermuter::threadpermuter ${AICXX_OBJECTS_LIST})

add_executable(RWLock_test RWLock_test.cxx)
target_link_libraries(RWLock_test ThreadPermuter::threadpermuter ${AICXX_OBJECTS_LIST})

