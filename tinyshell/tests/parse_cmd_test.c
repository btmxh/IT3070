#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse_cmd.h"

void check_testcase_generic(const char* cmd, const char** argv, int fg) {
  command_parse_result result;
  char* error = NULL;
  if(parse_command(cmd, &result, &error)) {
    assert(error == NULL && "error should not be set");
    int i = 0;
    while(1) {
      if(result.argv[i] == NULL && argv[i] == NULL) {
        break;
      }
      assert(result.argv[i]);
      assert(argv[i]);
      assert(strcmp(result.argv[i], argv[i]) == 0);
      ++i;
    }

    assert(result.foreground == fg || fg == -1);

    command_parse_result_free(&result);
  } else {
    assert(argv == NULL);
    free(error);
  }

}

void check_testcase(const char* cmd, const char** argv) {
  check_testcase_generic(cmd, argv, -1);
}

void check_testcase_bg(const char* cmd, const char** argv) {
  check_testcase_generic(cmd, argv, 0);
}

int main() {
  check_testcase("", (const char*[]) {NULL});
  check_testcase("  ", (const char*[]) {NULL});
  check_testcase("echo", (const char*[]) {"echo", NULL});
  check_testcase("echo 123", (const char*[]) {"echo", "123", NULL});
  check_testcase("echo 123 hello world  ", (const char*[]) {"echo", "123", "hello", "world", NULL});
  check_testcase("echo 123 \"hello world\"", (const char*[]) {"echo", "123", "hello world", NULL});
  check_testcase("echo 123 \"hello\"a\"world\"", (const char*[]) {"echo", "123", "helloaworld", NULL});
  check_testcase("echo \"", NULL);
  check_testcase("echo \'\"", NULL);
  check_testcase("echo \"\'\"", (const char*[]) {"echo", "\'", NULL});
#ifdef __unix__
  check_testcase("echo \'\"\'", (const char*[]) {"echo", "\"", NULL});
  check_testcase("echo \\\'", (const char*[]) {"echo", "\'", NULL});
  check_testcase("echo \\0", (const char*[]) {"echo", "0", NULL});
  check_testcase("echo \\ \\ ", (const char*[]) {"echo", "  ", NULL});
  check_testcase("echo \\ \\", NULL);
#endif

#ifdef WIN32
  check_testcase("echo ^\'", (const char*[]) {"echo", "\'", NULL});
  check_testcase("echo ^0", (const char*[]) {"echo", "0", NULL});
  check_testcase("echo ^ ^ ", (const char*[]) {"echo", "  ", NULL});
  check_testcase("echo ^ ^", NULL);
#endif
  check_testcase_bg("echo & ", (const char*[]){"echo", NULL});
  return 0;
}
