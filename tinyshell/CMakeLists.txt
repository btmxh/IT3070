# Declare the minimum required CMake version
cmake_minimum_required(VERSION 3.14)

# Define the project, we are using C (and a little bit of C++)
project(tinyshell LANGUAGES C CXX)

# Glob all source files and put them into the `shell_SOURCES` CMake variable
file(GLOB_RECURSE tinyshell_SOURCES CONFIGURE_DEPENDS libtinyshell/*.h libtinyshell/*.c)
file(GLOB_RECURSE tinyshell_TESTS CONFIGURE_DEPENDS tests/*.c)

# Glob platform source files
if(UNIX)
  file(GLOB_RECURSE tinyshell_PLATFORM_SOURCES CONFIGURE_DEPENDS libtinyshell/platform/unix/*.h libtinyshell/platform/unix/*.c)
  file(GLOB_RECURSE tinyshell_PLATFORM_TESTS CONFIGURE_DEPENDS tests/platform/unix/*.h tests/platform/unix/*.c)
elseif(WIN32)
  file(GLOB_RECURSE tinyshell_PLATFORM_SOURCES CONFIGURE_DEPENDS libtinyshell/platform/windows/*.h libtinyshell/platform/windows/*.c)
  file(GLOB_RECURSE tinyshell_PLATFORM_TESTS CONFIGURE_DEPENDS tests/platform/windows/*.h tests/platform/windows/*.c)
else()
  message(FATAL_ERROR "Unsupported platform")
endif()

list(APPEND tinyshell_SOURCES ${tinyshell_PLATFORM_SOURCES})
list(APPEND tinyshell_TESTS ${tinyshell_PLATFORM_TESTS})

# Declare an executable named `tinyshell`, with source files being the files globbed
# in the `tinyshell_SOURCES` variable
add_library(libtinyshell ${tinyshell_SOURCES})
target_include_directories(libtinyshell PUBLIC libtinyshell)

add_executable(tinyshell main.c)
target_link_libraries(tinyshell PUBLIC libtinyshell)

enable_testing()

foreach(TEST_FILE ${tinyshell_TESTS})
  cmake_path(SET path ${TEST_FILE})
  cmake_path(GET path FILENAME filename)
  cmake_path(REMOVE_EXTENSION filename OUTPUT_VARIABLE execname)
  add_executable(${execname} ${TEST_FILE})
  target_link_libraries(${execname} PUBLIC libtinyshell)
  add_test(NAME ${execname} COMMAND $<TARGET_FILE:${execname}>)
endforeach()


