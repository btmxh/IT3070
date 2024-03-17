#pragma once

typedef struct {} tinyshell;

int tinyshell_new(tinyshell* shell);
int tinyshell_run(tinyshell* shell);
void tinyshell_destroy(tinyshell* shell);
