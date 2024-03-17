#pragma once

typedef struct {
  int temp; // C requires a struct to have at least 1 member
} tinyshell;

int tinyshell_new(tinyshell* shell);
int tinyshell_run(tinyshell* shell);
void tinyshell_destroy(tinyshell* shell);
