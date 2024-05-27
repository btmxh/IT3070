#pragma once

#include "tinyshell.h"

int builtin_date(tinyshell* shell, int argc, char* argv[]);
int builtin_time(tinyshell* shell, int argc, char* argv[]);
int builtin_exit(tinyshell* shell, int argc, char* argv[]);
int builtin_help(tinyshell* shell, int argc, char* argv[]);
int builtin_ls(tinyshell* shell, int argc, char* argv[]);