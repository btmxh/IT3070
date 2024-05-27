#include <stdio.h>
#include <builtin.h>
#include <time.h>
#include <stdlib.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>

static int print_datetime(const char* format) {
    // get current time
    time_t current_time = time(NULL);
    struct tm *timeinfo = localtime(&current_time);

    // convert to string
    int size = 100; // Initial size of the buffer
    char *timestamp = NULL;
    do {
        free(timestamp);
        timestamp = malloc(size);
        if(timestamp == NULL) {
            return 1;
        }

        size *= 2;
    } while(strftime(timestamp, size, format, timeinfo) == 0);
    
    printf("%s\n", timestamp);
    free(timestamp); // Free the dynamically allocated memory
    return 0;
}

int builtin_date(tinyshell* shell, int argc, char* argv[]) {
    const char* format = "%Y-%m-%d";
    if (argc >= 2) {
        format = argv[1];
    }

    return print_datetime(format);
}

int builtin_time(tinyshell* shell, int argc, char* argv[]) {
    const char* format = "%H:%M:%S";
    if (argc >= 2) {
        format = argv[1];
    }

    return print_datetime(format);
}

int builtin_exit(tinyshell* shell, int argc, char* argv[]) {
    shell->exit = true;
    return 0;
}

int builtin_help(tinyshell* shell, int argc, char* argv[]) {
    printf("Hello World");
    return 0;
}


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
    return strcmp(*(const char **)a, *(const char **)b);
}

int builtin_ls(tinyshell* shell, int argc, char* argv[]) {
    struct dirent *pDirent;
    DIR *pDir;
    struct stat fileStat;
    char *entries[1000];
    int count = 0;
    int showDetails = 0;

    // Kiểm tra các tham số đầu vào
    if (argc > 3) {
        printf("Usage: %s [-l] <dirname>\n", argv[0]);
        return 1;
    }

    const char* dir_path = NULL;

    for(int i = 1; i < argc; i++) {
        const char* arg = argv[i];

        if(arg[0] == '-') { // arg là một option
            if(strcmp(arg, "-l") == 0) {
                showDetails = 1;
                continue;
            }

            printf("Invalid option: %s\n", arg);
            return 1;
        }

        if(dir_path == NULL) {
            dir_path = arg;
            continue;
        }

        printf("Trailing argument: %s\n", arg);
        return 1;
    }

    if(dir_path == NULL) {
        dir_path = ".";
    }

    pDir = opendir(dir_path);

    if (pDir == NULL) {
        printf("Cannot open directory '%s'\n", dir_path);
        return 1;
    }

    // Đọc các entry trong thư mục và lưu tên vào mảng
    while ((pDirent = readdir(pDir)) != NULL) {
        entries[count] = strdup(pDirent->d_name);
        count++;
    }

    closedir(pDir);

    // Sắp xếp các mục theo thứ tự bảng chữ cái
    qsort(entries, count, sizeof(char *), compare);

    // In ra tên các mục
    for (int i = 0; i < count; i++) {
        if (showDetails) {
            if (stat(entries[i], &fileStat) < 0) {
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
        printf("%s\n", entries[i]);
        free(entries[i]);  // Giải phóng bộ nhớ sau khi in
    }

    return 0;
}