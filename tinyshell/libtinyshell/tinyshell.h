#pragma once

typedef struct {
  int temp;
} tinyshell;

int tinyshell_new(tinyshell* shell);
int tinyshell_run(tinyshell* shell);
void tinyshell_destroy(tinyshell* shell);

const char* tinyshell_get_path_env(const tinyshell* shell);

char* get_current_directory();
