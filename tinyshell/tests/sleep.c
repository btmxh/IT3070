#include "run_script.h"

int main() {
  run_script("tests/scripts/sleep" SCRIPT_EXTENSION);
  return 0;
}
