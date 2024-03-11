# Declare the minimum required CMake version
cmake_minimum_required(VERSION 3.14)

# Define the project, we are using C (and a little bit of C++)
project(tinyshell LANGUAGES C CXX)

# Glob all source files and put them into the `shell_SOURCES` CMake variable
file(GLOB_RECURSE tinyshell_SOURCES CONFIGURE_DEPENDS src/**.h src/**.c)

# Declare an executable named `tinyshell`, with source files being the files globbed
# in the `tinyshell_SOURCES` variable
add_executable(tinyshell ${tinyshell_SOURCES})
