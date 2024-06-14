
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/time.h>

#include "logger.h"

#define MESSAGE_MAX   8196
#define LOG_INFO_MAX  512
#define FILE_SIZE_MAX (1024 * 1024 * 10)  // 10 mg

#define LEN(array) sizeof(array) / sizeof(array[0])

typedef struct DateTime {
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t ms;
    uint8_t week;
} DateTime;

static void get_datetime(DateTime *datetime) {
    struct timeval tv;
    time_t         rawtime  = time(NULL);
    struct tm     *timeinfo = localtime(&rawtime);

    datetime->month   = timeinfo->tm_mon + 1;
    datetime->day     = timeinfo->tm_mday;
    datetime->hour    = timeinfo->tm_hour;
    datetime->minutes = timeinfo->tm_min;
    datetime->seconds = timeinfo->tm_sec;
    datetime->week    = timeinfo->tm_yday / 7;

    if (gettimeofday(&tv, NULL) < 0)
        datetime->ms = 0;
    else
        datetime->ms = (uint8_t)(tv.tv_usec / 10000);
}

static FILE  *file          = NULL;
static size_t file_size     = 0;
static char   file_path[50] = { 0 };

static int update_file(void) {
    if (file != NULL && file_path[0] != 0 && file_size < FILE_SIZE_MAX &&
        !access(file_path, F_OK)) {
        return 0;
    }

    mkdir("logs", 0755);

    DIR           *dir;
    struct dirent *de;
    uint32_t       file_iter = 0;

    if ((dir = opendir("logs")) == NULL) {
        return 1;
    }

    while ((de = readdir(dir)) != NULL) {
        // ignore the ".." and "." dir and all the hidden stuff
        if (de->d_name[0] == '.') continue;

        uint32_t n = 0;
        if (sscanf(de->d_name, "%u.log", &n) < 1) continue;
        if (n > file_iter) file_iter = n;
    }

    snprintf(file_path, 50, "logs/%u.log", file_iter);
    if ((file = fopen(file_path, "a")) == NULL) return 1;

    file_size = ftell(file);
    if (file_size >= FILE_SIZE_MAX) {
        snprintf(file_path, 50, "logs/%u.log", file_iter + 1);
        if ((file = fopen(file_path, "a")) == NULL) return 1;
        file_size = 0;
    }

    return 0;
}

static char *get_tag(Flag flag, bool color) {
    if (color) {
        switch (flag) {
            // "\033[40m<\033[93mVERB\033[37m>\033[0m"
            case LF_VERB: return "<\033[93;40mVERB\033[0m>";
            case LF_INFO: return "<\033[34mINFO\033[0m>";
            case LF_WARN: return "<\033[33mWARN\033[0m>";
            case LF_DBUG: return "<\033[35mDBUG\033[0m>";
            case LF_EROR: return "<\033[31mEROR\033[0m>";
            case LF_BRAK: return "";
        }
    }

    switch (flag) {
        case LF_VERB: return "<VERB>";
        case LF_INFO: return "<INFO>";
        case LF_WARN: return "<WARN>";
        case LF_DBUG: return "<DBUG>";
        case LF_EROR: return "<EROR>";
        case LF_BRAK: return "";
    }

    return "NULL";
}

void logger(char *name, const Flag flag, const char *format, ...) {
    DateTime datetime;
    va_list  args;

    // SectorFile *sector = &SECTORS[index / 100];

    char message[MESSAGE_MAX];
    char info[LOG_INFO_MAX];
    char info_color[LOG_INFO_MAX];

    // format the message
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    // update the datetime
    get_datetime(&datetime);
    snprintf(
        info,
        sizeof(info),
        "%02d-%02d %02d:%02d:%02d.%03d %s",
        datetime.month,
        datetime.day,
        datetime.hour,
        datetime.minutes,
        datetime.seconds,
        datetime.ms,
        get_tag(flag, false)
    );

    snprintf(
        info_color,
        sizeof(info_color),
        "\033[32m%02d-%02d %02d:%02d:%02d.%03d\033[0m %s",
        datetime.month,
        datetime.day,
        datetime.hour,
        datetime.minutes,
        datetime.seconds,
        datetime.ms,
        get_tag(flag, true)
    );

    // log to screen
    if (flag == LF_BRAK)
        printf("\n");
    else {
        printf("%s [\033[36m%s\033[0m] %s\n", info_color, name, message);
    }

    if (flag == LF_VERB) return;
    update_file();

    if (file != NULL) {
        if (flag == LF_BRAK)
            fprintf(file, "\n");
        else
            fprintf(file, "%s: %s %s\n", name, info, message);
        fflush(file);
        file_size = ftell(file);
    } else {
        printf("invalid log file\n");
    }
}

int logger_setup(void) {
    return update_file();
}

void logger_clean(void) {
    if (file != NULL) fclose(file);
}
