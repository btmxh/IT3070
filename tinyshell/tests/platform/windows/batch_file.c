#include "../../run_script.h"

#include <direct.h>

int main() {
  _chdir(ROOT_TEST);
  run_script("tests/scripts/start_echo.tbat");
  return 0;
}
