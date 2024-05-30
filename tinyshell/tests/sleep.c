#include "run_script.h"

int main() {
// ping on github actions does not work
  run_script(ROOT_TEST "tests/scripts/sleep" SCRIPT_EXTENSION);
  return 0;
}
