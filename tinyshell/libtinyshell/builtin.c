#include "builtin.h"
#include "parse_cmd.h"
#include "tinyshell.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef WIN32
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
  printf("Hello World");
  return 0;
}

#ifdef WIN32
int builtin_ls(tinyshell *shell, int argc, char *argv[]) {
  const char *dir = argc <= 1 ? NULL : argv[1];

  char *cwd = get_current_directory();
  if (!cwd) {
    puts("unable to retrieve current directory");
    return 1;
  }

  printf("\nDirectory of %s\n\n", cwd);
  free(cwd);

  char *pattern = printf_to_string("%s\\*", dir);
  if (!pattern) {
    return 1;
  }

  WIN32_FIND_DATA file_data;
  HANDLE find = FindFirstFile(pattern, &file_data);
  if (find == INVALID_HANDLE_VALUE) {
    free(pattern);
    return 0;
  }

  do {
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

int builtin_ls(tinyshell *shell, int argc, char *argv[]) {
  DIR *pDir;
  struct stat fileStat;
  int showDetails = 0;
  struct dirent *entries = NULL;
  int entries_len = 0, entries_cap = 0;

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

  pDir = opendir(dir_path);

  if (pDir == NULL) {
    printf("Cannot open directory '%s'\n", dir_path);
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
    if (showDetails) {
      if (stat(entries[i].d_name, &fileStat) < 0) {
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
