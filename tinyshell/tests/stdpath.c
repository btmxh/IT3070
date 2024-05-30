#include "run_script.h"

int main() {
  run_script(ROOT_TEST "tests/scripts/stdpath" SCRIPT_EXTENSION);
  return 0;
}
