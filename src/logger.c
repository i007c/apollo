
#include "common.h"
#include "logger.h"


#define MESSAGE_MAX 8196
#define INFO_MAX 512
#define PATH_MAX 40


typedef struct DateTime {
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t ms;
    uint8_t week;
} DateTime;

typedef struct SectorFile {
    FILE *file;
    const char name[PATH_MAX - 15];
    char path[PATH_MAX];
} SectorFile;



static uint8_t sector_file_day = 0;

static SectorFile SECTORS[] = {
    [SECTOR_MAIN / 100]   = { NULL, "main",   "" },
    // [SECTOR_USER / 100]   = { NULL, "user",   "" },
    // [SECTOR_ADMIN / 100]  = { NULL, "admin",  "" },
    // [SECTOR_SERVER / 100] = { NULL, "server", "" },
    // [SECTOR_EATERY / 100] = { NULL, "eatery", "" },
    // [SECTOR_DETAIL / 100] = { NULL, "detail", "" },
};

static char* SUB_SECTOR_NAMES[] = {
    [SECTOR_MAIN_APOLLO]         = "apollo",
    // [SECTOR_USER_PHONE]       = "phone",
    // [SECTOR_EATERY_DISH]      = "dish",
    // [SECTOR_EATERY_REVIEW]    = "review",
};

static void make_dirs() {
    char dirname[PATH_MAX - 10];

    mkdir("logs", 0755);

    for (uint8_t i = 0; i < SECTOR_LENGTH / 100; i++) {
        snprintf(dirname, sizeof(dirname), "logs/%s", SECTORS[i].name);
        mkdir(dirname, 0755);
    }
}

static void get_datetime(DateTime *datetime) {
    struct timeval tv;
    time_t rawtime = time(NULL);
    struct tm *timeinfo = localtime(&rawtime);

    datetime->month = timeinfo->tm_mon + 1;
    datetime->day = timeinfo->tm_mday;
    datetime->hour = timeinfo->tm_hour;
    datetime->minutes = timeinfo->tm_min;
    datetime->seconds = timeinfo->tm_sec;
    datetime->week = timeinfo->tm_yday / 7;

    if (gettimeofday(&tv, NULL) < 0)
        datetime->ms = 0;
    else
        datetime->ms = (uint8_t)(tv.tv_usec / 10000);
}

static void update_sectors(DateTime *datetime) {
    bool called_make_dirs = false;
    FILE *fd = NULL;

    for (uint8_t i = 0; i < SECTOR_LENGTH / 100; i++) {
        snprintf(
            SECTORS[i].path, PATH_MAX, "logs/%s/%02d.log", 
            SECTORS[i].name, datetime->week
        );

        fd = fopen(SECTORS[i].path, "a");
        if (!called_make_dirs && fd == NULL && errno == ENOENT) {
            make_dirs();
            called_make_dirs = true;
            fd = fopen(SECTORS[i].path, "a");
        }

        if (SECTORS[i].file != NULL) fclose(SECTORS[i].file);

        SECTORS[i].file = fd;
    }

    sector_file_day = datetime->day;
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
        case LF_VERB:  return "<VERB>";
        case LF_INFO:  return "<INFO>";
        case LF_WARN:  return "<WARN>";
        case LF_DBUG:  return "<DBUG>";
        case LF_EROR:  return "<EROR>";
        case LF_BRAK:  return "";
    }

    return "NULL";
}

void logger(const Sector index, const Flag flag, const char *format, ...) {

    DateTime datetime;
    va_list args;

    SectorFile *sector = &SECTORS[index / 100];

    char message[MESSAGE_MAX];
    char info[INFO_MAX];
    char info_color[INFO_MAX];

    // format the message
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    // update the datetime
    get_datetime(&datetime);
    snprintf(
        info, sizeof(info), "%02d-%02d %02d:%02d:%02d.%03d %s", 
        datetime.month, datetime.day, datetime.hour,
        datetime.minutes, datetime.seconds, datetime.ms,
        get_tag(flag, false)
    );

    snprintf(
        info_color, sizeof(info_color),
        "\033[32m%02d-%02d %02d:%02d:%02d.%03d\033[0m %s", 
        datetime.month, datetime.day, datetime.hour,
        datetime.minutes, datetime.seconds, datetime.ms,
        get_tag(flag, true)
    );

    // log to screen
    if (flag == LF_BRAK)
        printf("\n");
    else {
        printf("%s [\033[36m%s", info_color, sector->name);
        if (index % 100) printf(".%s", SUB_SECTOR_NAMES[index]);
        printf("\033[0m] %s\n", message);
    }

    // log to file
    if (datetime.day != sector_file_day || access(sector->path, F_OK))
        update_sectors(&datetime);

    if (flag == LF_VERB)
        return;

    if (sector->file != NULL) {
        if (flag == LF_BRAK)
            fprintf(sector->file, "\n");
        else
            fprintf(sector->file, "%s %s\n", info, message);
        fflush(sector->file);
    } else {
        update_sectors(&datetime);
    }
    
}


status_t logger_setup(void) {
    DateTime datetime;
    get_datetime(&datetime);
    make_dirs();
    update_sectors(&datetime);
    
    return STS_SUCCESS;
}

void logger_clean(void) {
    for (uint8_t i = 0; i < 1; i++) {
        if (SECTORS[i].file != NULL)
            fclose(SECTORS[i].file);
    }
}
