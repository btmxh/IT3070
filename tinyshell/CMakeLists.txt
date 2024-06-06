# Declare the minimum required CMake version
cmake_minimum_required(VERSION 3.14)

# Define the project, we are using C (and a little bit of C++)
project(tinyshell LANGUAGES C CXX)

option(CI_TEST "disable certain tests that could not be run on CI environments" OFF)
string(REGEX REPLACE "\\\\" "\\\\\\\\" ROOT_TEST "${ROOT_TEST}")

# Glob all source files and put them into the `shell_SOURCES` CMake variable
file(GLOB tinyshell_SOURCES CONFIGURE_DEPENDS libtinyshell/*.h libtinyshell/*.c)
file(GLOB tinyshell_TESTS CONFIGURE_DEPENDS tests/*.c)

add_subdirectory(tinycthread)

# Glob platform source files
if(UNIX)
  file(GLOB tinyshell_PLATFORM_SOURCES CONFIGURE_DEPENDS libtinyshell/platform/unix/*.h libtinyshell/platform/unix/*.c)
  file(GLOB tinyshell_PLATFORM_TESTS CONFIGURE_DEPENDS tests/platform/unix/*.c)
elseif(WIN32)
  file(GLOB tinyshell_PLATFORM_SOURCES CONFIGURE_DEPENDS libtinyshell/platform/windows/*.h libtinyshell/platform/windows/*.c)
  file(GLOB tinyshell_PLATFORM_TESTS CONFIGURE_DEPENDS tests/platform/windows/*.c)
else()
  message(FATAL_ERROR "Unsupported platform")
endif()

list(APPEND tinyshell_SOURCES ${tinyshell_PLATFORM_SOURCES})
list(APPEND tinyshell_TESTS ${tinyshell_PLATFORM_TESTS})

# Declare an executable named `tinyshell`, with source files being the files globbed
# in the `tinyshell_SOURCES` variable
add_library(libtinyshell ${tinyshell_SOURCES})
target_include_directories(libtinyshell PUBLIC libtinyshell)
target_link_libraries(libtinyshell PUBLIC tinycthread)

if(WIN32)
  target_link_libraries(libtinyshell PRIVATE shlwapi)
endif()

add_executable(tinyshell main.c)
target_link_libraries(tinyshell PUBLIC libtinyshell)

enable_testing()
set(MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full --error-exitcode=1")
include(CTest)

foreach(TEST_FILE ${tinyshell_TESTS})
  cmake_path(SET path ${TEST_FILE})
  cmake_path(GET path FILENAME filename)
  cmake_path(REMOVE_EXTENSION filename OUTPUT_VARIABLE execname)
  add_executable(${execname} ${TEST_FILE})
  target_link_libraries(${execname} PUBLIC libtinyshell)
  add_test(NAME ${execname} COMMAND $<TARGET_FILE:${execname}>)
  if(CI_TEST)
    target_compile_definitions(${execname} PRIVATE CI_TEST)
  endif()
  target_compile_definitions(${execname} PRIVATE -DROOT_TEST="${ROOT_TEST}/")
endforeach()

