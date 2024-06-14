
#ifndef __PLUTUS_LOGGER_H__
#define __PLUTUS_LOGGER_H__

#define LOG_TRACE_LVL1(LINE) #LINE
#define LOG_TRACE_LVL2(LINE) LOG_TRACE_LVL1(LINE)
#define LOG_THROW_LOCATION   "[" __FILE__ ":" LOG_TRACE_LVL2(__LINE__) "] "

typedef enum {
    LF_VERB,
    LF_INFO,
    LF_WARN,
    LF_DBUG,
    LF_EROR,
    LF_BRAK,
} Flag;

#define log_verbose(...) logger(LOG_NAME, LF_VERB, __VA_ARGS__)
#define log_info(...)    logger(LOG_NAME, LF_INFO, __VA_ARGS__)
#define log_warn(...)    logger(LOG_NAME, LF_WARN, __VA_ARGS__)
#define log_debug(...)   logger(LOG_NAME, LF_DBUG, __VA_ARGS__)
#define log_error(...)   logger(LOG_NAME, LF_EROR, __VA_ARGS__)
#define log_trace(...)   logger(LOG_NAME, LF_EROR, LOG_THROW_LOCATION __VA_ARGS__)
#define log_break()      logger(LOG_NAME, LF_BRAK, "")

#define logh_verbose(...) logger(H->B.name, LF_VERB, __VA_ARGS__)
#define logh_info(...)    logger(H->B.name, LF_INFO, __VA_ARGS__)
#define logh_warn(...)    logger(H->B.name, LF_WARN, __VA_ARGS__)
#define logh_debug(...)   logger(H->B.name, LF_DBUG, __VA_ARGS__)
#define logh_error(...)   logger(H->B.name, LF_EROR, __VA_ARGS__)
#define logh_trace(...)   logger(H->B.name, LF_EROR, LOG_THROW_LOCATION __VA_ARGS__)
#define logh_break()      logger(H->B.name, LF_BRAK, "")

void logger(char *name, const Flag flag, const char *format, ...);

int  logger_setup(void);
void logger_clean(void);

#endif  // __PLUTUS_LOGGER_H__
