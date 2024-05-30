#include "run_script.h"

int main() {
// ping on github actions does not work
#if !defined(WIN32) || !defined(CI_TEST)
  run_script(ROOT_TEST "tests/scripts/sleep" SCRIPT_EXTENSION);
#endif
  return 0;
}
