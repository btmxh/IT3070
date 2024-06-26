#include "builtin.h"
#include "parse_cmd.h"
#include "process.h"
#include "tinyshell.h"
#include "utils.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#include <fileapi.h>
#include <handleapi.h>
#include <timezoneapi.h>
#include <winnt.h>
#else
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

int try_run_builtin(tinyshell *shell, command_parse_result *result,
                    int *status_code) {
  const char *arg0 = result->argv[0];
  if (strcmp(arg0, "cd") == 0) {
    *status_code = builtin_cd(shell, result->argc, result->argv);
    command_parse_result_free(result);
    return 1;
  } else if (strcmp(arg0, "pwd") == 0) {
    *status_code = builtin_pwd(shell, result->argc, result->argv);
    command_parse_result_free(result);
    return 1;
  } else if (strcmp(arg0, "date") == 0) {
    *status_code = builtin_date(shell, result->argc, result->argv);
    command_parse_result_free(result);
    return 1;
  } else if (strcmp(arg0, "time") == 0) {
    *status_code = builtin_time(shell, result->argc, result->argv);
    command_parse_result_free(result);
    return 1;
  } else if (strcmp(arg0, "exit") == 0) {
    *status_code = builtin_exit(shell, result->argc, result->argv);
    command_parse_result_free(result);
    return 1;
  } else if (strcmp(arg0, "help") == 0) {
    *status_code = builtin_help(shell, result->argc, result->argv);
    command_parse_result_free(result);
    return 1;
  } else if (strcmp(arg0, "ls") == 0 || strcmp(arg0, "dir") == 0) {
    *status_code = builtin_ls(shell, result->argc, result->argv);
    command_parse_result_free(result);
    return 1;
  } else if (strcmp(arg0, "jobs") == 0 || strcmp(arg0, "list") == 0) {
    *status_code = builtin_jobs(shell, result->argc, result->argv);
    command_parse_result_free(result);
    return 1;
  } else if (strcmp(arg0, "kill") == 0) {
    *status_code = builtin_kill(shell, result->argc, result->argv);
    command_parse_result_free(result);
    return 1;
  } else if (strcmp(arg0, "stop") == 0) {
    *status_code = builtin_stop(shell, result->argc, result->argv);
    command_parse_result_free(result);
    return 1;
  } else if (strcmp(arg0, "resume") == 0) {
    *status_code = builtin_resume(shell, result->argc, result->argv);
    command_parse_result_free(result);
    return 1;
  } else if (strcmp(arg0, "addpath") == 0) {
    *status_code = builtin_addpath(shell, result->argc, result->argv);
    command_parse_result_free(result);
    return 1;
  } else if (strcmp(arg0, "setpath") == 0) {
    *status_code = builtin_setpath(shell, result->argc, result->argv);
    command_parse_result_free(result);
    return 1;
  } else if (strcmp(arg0, "path") == 0) {
    *status_code = builtin_path(shell, result->argc, result->argv);
    command_parse_result_free(result);
    return 1;
  }

  return 0;
}

int builtin_cd(tinyshell *shell, int argc, char *argv[]) {
  if (argc != 2) {
    puts("usage: cd <directory>");
    return 1;
  }

  if (POSIX_WIN32(chdir)(argv[1])) {
    printf("unable to change directory to %s\n", argv[1]);
    return 1;
  }

  return 0;
}

int builtin_pwd(tinyshell *shell, int argc, char *argv[]) {
  char *cwd = get_current_directory();
  if (!cwd) {
    return 1;
  }

  puts(cwd);
  free(cwd);
  return 0;
}

static int print_datetime(const char *format) {
  // get current time
  time_t current_time = time(NULL);
  struct tm *timeinfo = localtime(&current_time);

  // convert to string
  int size = 100; // Initial size of the buffer
  char *timestamp = NULL;
  do {
    free(timestamp);
    timestamp = malloc(size);
    if (timestamp == NULL) {
      return 1;
    }

    size *= 2;
  } while (strftime(timestamp, size, format, timeinfo) == 0);

  printf("%s\n", timestamp);
  free(timestamp); // Free the dynamically allocated memory
  return 0;
}

int builtin_date(tinyshell *shell, int argc, char *argv[]) {
  const char *format = "%Y-%m-%d";
  if (argc >= 2) {
    format = argv[1];
  }

  return print_datetime(format);
}

int builtin_time(tinyshell *shell, int argc, char *argv[]) {
  const char *format = "%H:%M:%S";
  if (argc >= 2) {
    format = argv[1];
  }

  return print_datetime(format);
}

int builtin_exit(tinyshell *shell, int argc, char *argv[]) {
  shell->exit = 1;
  return 0;
}

int builtin_help(tinyshell *shell, int argc, char *argv[]) {
  printf(
// clang-format off
"= tinyshell (pre-release version)\n"
"Git reposistory: https://github.com/btmxh/IT3070 (in the `tinyshell` directory)\n"
"\n"
"= Builtin commands\n"
"- `exit`      - exit this shell\n"
"- `help`      - print help\n"
"- `cd`        - change directory\n"
"- `pwd`       - print working directory\n"
"- `date/time` - print current date/time\n"
"                provide an optional format to customize the output\n"
"                the format is interpreted as defined here:\n"
"                (https://en.cppreference.com/w/c/chrono/strftime)\n"
"                the only differences between date/time are the default format\n"
"- `ls/dir`    - print information about files in a directory\n"
"                (default: the current working directory).\n"
"                add option `-l` to print more details\n"
"- `jobs`      - print all currently active jobs (background process)\n"
"                to create a new job, append an ampersand (&) to the command\n"
"                when launching a process\n"
"- `kill`      - kill jobs specified in the arguments\n"
"- `stop`      - stop jobs specified in the arguments\n"
"- `resume`    - resume jobs specified in the arguments\n"
"- `setpath`   - set the shell PATH to the value specified in the argument\n"
"                note that when processes are started, the shell will not pass\n"
"                the PATH environment variable to the process\n"
"- `addpath`   - add paths to the shell PATH. use `setpath` to clear this value\n"
"- `path`      - print the current shell PATH\n"
"                the separator between paths is system-dependent:\n"
"                Win32 - ';', Unix/POSIX - ':'\n"
"\n"
"= Jobs and processes\n"
"\n"
"Enter a command to launch a new process using that command.\n"
"\n"
"The arguments are internally processed to match the behavior of the current OS.\n"
"\n"
"Add an ampersand ('&') to the end of the command to launch as a job\n"
"(background process).\n"
"\n"
"Use CTRL+C to cancel a currently running process. This is a SIGINT on Unix, so\n"
"the process could catch the signal and refuse to terminate.\n"
"\n"
"= Scripts\n"
"\n"
"For Windows batch files (*.bat), tinyshell uses cmd.exe internally to process\n"
"those scripts.\n"
"\n"
"Custom mini scripting languages are also supported: tinyshell script (.tsh) and\n"
"tinyshell batch script (.tbat). The shell simply execute the scripts line by\n"
"line. *.tsh is only supported on Unix/POSIX, and *.tbat is only supported on\n"
"Windows.\n"
// clang-format on
  );
  return 0;
}

#ifdef _WIN32
static int exec_ls(const char* dir, int show_details) {
  if(show_details) {
    printf("\nDirectory of %s\n\n", dir);
  }

  char *pattern = printf_to_string("%s\\*", dir);
  if (!pattern) {
    return 1;
  }

  WIN32_FIND_DATA file_data;
  HANDLE find = FindFirstFile(pattern, &file_data);
  if (find == INVALID_HANDLE_VALUE) {
    free(pattern);
    return 1;
  }

  do {
    if(!show_details) {
      puts(file_data.cFileName);
      continue;
    }

    SYSTEMTIME last_access_time;
    if (!FileTimeToSystemTime(&file_data.ftLastAccessTime, &last_access_time)) {
      puts("error converting last access time");
      free(pattern);
      return 1;
    }

    int hour = last_access_time.wHour;
    if (hour == 0) {
      hour = 12;
    } else if (hour > 12) {
      hour -= 12;
    }

    ULARGE_INTEGER ul;
    ul.HighPart = file_data.nFileSizeHigh;
    ul.LowPart = file_data.nFileSizeLow;

    if (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      printf("%02d/%02d/%04d  %02d:%02d %cM   <DIR>        %s\n",
             last_access_time.wMonth, last_access_time.wDay,
             last_access_time.wYear, hour, last_access_time.wMinute,
             last_access_time.wHour < 12 ? 'A' : 'P', file_data.cFileName);
    } else {
      printf("%02d/%02d/%04d  %02d:%02d %cM        %7lld %s\n",
             last_access_time.wMonth, last_access_time.wDay,
             last_access_time.wYear, hour, last_access_time.wMinute,
             last_access_time.wHour < 12 ? 'A' : 'P', ul.QuadPart,
             file_data.cFileName);
    }
  } while (FindNextFile(find, &file_data));
  free(pattern);

  return 0;
}
#else
// Hàm để hiển thị quyền truy cập tệp tin
static void printPermissions(mode_t mode) {
  char permissions[11];
  permissions[0] = (S_ISDIR(mode)) ? 'd' : '-';
  permissions[1] = (mode & S_IRUSR) ? 'r' : '-';
  permissions[2] = (mode & S_IWUSR) ? 'w' : '-';
  permissions[3] = (mode & S_IXUSR) ? 'x' : '-';
  permissions[4] = (mode & S_IRGRP) ? 'r' : '-';
  permissions[5] = (mode & S_IWGRP) ? 'w' : '-';
  permissions[6] = (mode & S_IXGRP) ? 'x' : '-';
  permissions[7] = (mode & S_IROTH) ? 'r' : '-';
  permissions[8] = (mode & S_IWOTH) ? 'w' : '-';
  permissions[9] = (mode & S_IXOTH) ? 'x' : '-';
  permissions[10] = '\0';

  printf("%s ", permissions);
}

// Hàm so sánh cho qsort
static int compare(const void *a, const void *b) {
  const struct dirent *ea = a, *eb = b;
  return strcmp(ea->d_name, eb->d_name);
}

static int exec_ls(const char* dir, int show_details) {
  DIR *pDir;
  struct stat fileStat;
  struct dirent *entries = NULL;
  int entries_len = 0, entries_cap = 0;

  pDir = opendir(dir);

  if (pDir == NULL) {
    printf("cannot open directory '%s'\n", dir);
    return 1;
  }

  // Đọc các entry trong thư mục và lưu tên vào mảng
  struct dirent *entry;
  while ((entry = readdir(pDir)) != NULL) {
    vecpush(&entries, &entries_len, &entries_cap, sizeof *entry, entry, 1);
  }

  closedir(pDir);

  // Sắp xếp các mục theo thứ tự bảng chữ cái
  qsort(entries, entries_len, sizeof *entries, compare);

  // In ra tên các mục
  printf("total %d\n", entries_len);
  for (int i = 0; i < entries_len; i++) {
    if (show_details) {
      char* name = printf_to_string("%s/%s", dir, entries[i].d_name);
      if(!name) {
        puts("error formatting path");
        continue;
      }

      if (stat(name, &fileStat) < 0) {
        perror("stat");
        continue;
      }
      printPermissions(fileStat.st_mode);
      printf("%ld ", fileStat.st_nlink);
      printf("%s ", getpwuid(fileStat.st_uid)->pw_name);
      printf("%s ", getgrgid(fileStat.st_gid)->gr_name);
      printf("%5ld ", fileStat.st_size);

      char timeBuf[80];
      struct tm *timeInfo = localtime(&fileStat.st_mtime);
      strftime(timeBuf, sizeof(timeBuf), "%m-%d-%Y", timeInfo);
      printf("%s ", timeBuf);
    }
    printf("%s\n", entries[i].d_name);
  }

  free(entries); // Giải phóng bộ nhớ sau khi in
  return 0;
}
#endif
int builtin_ls(tinyshell *shell, int argc, char *argv[]) {
  int showDetails = 0;

  // Kiểm tra các tham số đầu vào
  if (argc > 3) {
    printf("Usage: %s [-l] <dirname>\n", argv[0]);
    return 1;
  }

  const char *dir_path = NULL;

  for (int i = 1; i < argc; i++) {
    const char *arg = argv[i];

    if (arg[0] == '-') { // arg là một option
      if (strcmp(arg, "-l") == 0) {
        showDetails = 1;
        continue;
      }

      printf("Invalid option: %s\n", arg);
      return 1;
    }

    if (dir_path == NULL) {
      dir_path = arg;
      continue;
    }

    printf("Trailing argument: %s\n", arg);
    return 1;
  }

  if (dir_path == NULL) {
    dir_path = ".";
  }

  return exec_ls(dir_path, showDetails);
}

int builtin_jobs(tinyshell *shell, int argc, char *argv[]) {
  tinyshell_lock_bg_procs(shell);
  for (int i = 0; i < shell->bg_cap; ++i) {
    if (shell->bg[i].status != BG_PROCESS_EMPTY &&
        shell->bg[i].status != BG_PROCESS_FINISHED) {
      printf("job %%%d (%s): %s\n", i + 1,
             shell->bg[i].status == BG_PROCESS_RUNNING ? "running" : "stopped",
             shell->bg[i].cmd);
    }
  }
  tinyshell_unlock_bg_procs(shell);

  return 0;
}

static int parse_job_identifier(const tinyshell *shell, const char *job,
                                bg_process **p) {
  int job_index;
  if (job[0] != '%') {
    printf("invalid job identifier: %s\n", job);
    return 0;
  }

  char *end;
  errno = 0;
  job_index = (int)strtol(&job[1], &end, 10) - 1;
  if (end != job + strlen(job) || errno) {
    printf("invalid job identifier: %s\n", job);
    return 0;
  }

  if (job_index < 0 || job_index >= shell->bg_cap ||
      shell->bg[job_index].status == BG_PROCESS_EMPTY) {
    printf("job not found: %s\n", job);
    return 0;
  }

  if (p) {
    *p = &shell->bg[job_index];
  }

  return 1;
}

int builtin_kill(tinyshell *shell, int argc, char *argv[]) {
  tinyshell_lock_bg_procs(shell);
  for (int i = 1; i < argc; ++i) {
    bg_process *p;
    if (!parse_job_identifier(shell, argv[i], &p)) {
      return 1;
    }

    int status_code;
    if (!process_kill(&p->p)) {
      printf("unable to kill job %s\n", argv[i]);
      return 1;
    }

    tinyshell_unlock_bg_procs(shell);
    thrd_join(p->thread, NULL);
    p->status = BG_PROCESS_EMPTY;
  }
  tinyshell_unlock_bg_procs(shell);

  return 0;
}

int builtin_stop(tinyshell *shell, int argc, char *argv[]) {
  tinyshell_lock_bg_procs(shell);
  for (int i = 1; i < argc; ++i) {
    bg_process *p;
    if (!parse_job_identifier(shell, argv[i], &p)) {
      return 1;
    }

    if (p->status == BG_PROCESS_STOPPED) {
      printf("job %s is already stopped\n", argv[i]);
      continue;
    }

    if (!process_suspend(&p->p)) {
      printf("unable to suspend job %s\n", argv[i]);
      return 1;
    }

    p->status = BG_PROCESS_STOPPED;
  }
  tinyshell_unlock_bg_procs(shell);

  return 0;
}

int builtin_resume(tinyshell *shell, int argc, char *argv[]) {
  tinyshell_lock_bg_procs(shell);
  for (int i = 1; i < argc; ++i) {
    bg_process *p;
    if (!parse_job_identifier(shell, argv[i], &p)) {
      return 1;
    }

    if (p->status == BG_PROCESS_RUNNING) {
      printf("job %s is already running\n", argv[i]);
      continue;
    }

    if (!process_resume(&p->p)) {
      printf("unable to resume job %s\n", argv[i]);
      return 1;
    }

    p->status = BG_PROCESS_RUNNING;
  }
  tinyshell_unlock_bg_procs(shell);

  return 0;
}

static int add_path(tinyshell *shell, char **append, int free_path) {
#ifdef _WIN32
#define DELIM ";"
#else
#define DELIM ":"
#endif
  if (!shell->path) {
    shell->path = *append;
    *append = NULL;
    return 1;
  }

  char *new_path = printf_to_string("%s" DELIM "%s", shell->path, *append);
  if (!new_path) {
    if (free_path) {
      free(shell->path);
    }

    return 0;
  }

  if (free_path) {
    free(shell->path);
  }

  shell->path = new_path;

  return 1;
}

int builtin_addpath(tinyshell *shell, int argc, char *argv[]) {
  char *old_path = shell->path;
  for (int i = 1; i < argc; ++i) {
    if (!add_path(shell, &argv[i], i != 1)) {
      goto fail_add_path;
    }
  }

  free(old_path);
  return 0;

fail_add_path:
  shell->path = old_path;
  return 1;
}

int builtin_setpath(tinyshell *shell, int argc, char *argv[]) {
  if (argc != 2) {
    printf("usage: %s <new path>", argc > 0 ? argv[0] : "setpath");
  }

  free(shell->path);
  shell->path = argv[1];
  argv[1] = NULL;
  return 0;
}

int builtin_path(tinyshell *shell, int argc, char *argv[]) {
  puts(tinyshell_get_path_env(shell));
  return 0;
}
