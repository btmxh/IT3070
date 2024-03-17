#pragma once

typedef struct {} tinyshell;

int tinyshell_new(tinyshell* shell);
int tinyshell_run(tinyshell* shell);
void tinyshell_destroy(tinyshell* shell);

const char* tinyshell_get_current_directory(const tinyshell* shell);
const char* tinyshell_get_path_env(const tinyshell* shell);
