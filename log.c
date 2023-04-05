#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef WIN32
#include <windows.h>
#include <sys/timeb.h>
#else
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include "log.h"
#include "log_cfg.h"

#ifdef CONFIG_LOG_FILE_ENABLE
#ifndef CONFIG_LOG_FILE
#error "Please provide a output file name when log file enabled [CONFIG_LOG_FILE_ENABLE]!"
#endif
#endif

#define MAX_TIMESTR_LEN (24)
#define MAX_PID_LEN (10)
#define MAX_FILENAME_LEN (32)

#ifdef WIN32
#else
static pthread_mutex_t g_mutex;
#endif

#ifdef CONFIG_LOG_FILE_ENABLE
static FILE *g_logfile = NULL;
#endif

typedef struct {
    char tag;
    const char *level;
} LevelObject;

static const LevelObject g_level[] = {
        {'A', "ASSERT"},
        {'E', "ERROR"},
        {'W', "WARN"},
        {'I', "INFO"},
        {'D', "DEBUG"},
        {'V', "VERBOSE"},
};

static bool g_log_initialized = false;

/**
 * get current time interface
 *
 * @return current time
 */
static const char *log_get_time(void)
{
    static char cur_system_time[MAX_TIMESTR_LEN] = { 0 };

#ifdef WIN32
    SYSTEMTIME currTime;
    GetLocalTime(&currTime);
    snprintf(cur_system_time, MAX_TIMESTR_LEN, "%02d-%02d %02d:%02d:%02d.%03d", currTime.wMonth, currTime.wDay,
             currTime.wHour, currTime.wMinute, currTime.wSecond, currTime.wMilliseconds);
#else
    struct timeval tv;
    struct tm* t;
    gettimeofday(&tv, NULL);
    t = localtime(&tv.tv_sec);
    snprintf(cur_system_time, MAX_TIMESTR_LEN, "%02d-%02d %02d:%02d:%02d.%03ld",
                    1 + t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec / 1000);
#endif

    return cur_system_time;
}

/**
 * TODO implement
 * get format time
 * @return
 */
static const char *log_get_format_time(const char *fmt)
{
    static char cur_system_time[MAX_TIMESTR_LEN] = { 0 };

#ifdef WIN32
    SYSTEMTIME currTime;
    GetLocalTime(&currTime);
    snprintf(cur_system_time, MAX_TIMESTR_LEN, "%02d-%02d %02d:%02d:%02d.%03d", currTime.wMonth, currTime.wDay,
             currTime.wHour, currTime.wMinute, currTime.wSecond, currTime.wMilliseconds);
#else
    struct timeval tv;
    struct tm* t;
    gettimeofday(&tv, NULL);
    t = localtime(&tv.tv_sec);
    snprintf(cur_system_time, MAX_TIMESTR_LEN, "%02d-%02d %02d:%02d:%02d.%03ld",
                    1 + t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec / 1000);
#endif

    return cur_system_time;
}

/**
 * get current process name interface
 *
 * @return current process name
 */
static const char *log_get_p_info(void)
{
    static char cur_process_info[MAX_PID_LEN] = { 0 };

#ifdef WIN32
    snprintf(cur_process_info, MAX_PID_LEN, "pid:%04ld", GetCurrentProcessId());
#else
    snprintf(cur_process_info, MAX_PID_LEN, "pid:%04d", getpid());
#endif

    return cur_process_info;
}

/**
 * get current thread name interface
 *
 * @return current thread name
 */
const char *log_get_t_info(void)
{
    static char cur_thread_info[MAX_PID_LEN] = { 0 };

#ifdef WIN32
    snprintf(cur_thread_info, MAX_PID_LEN, "tid:%04ld", GetCurrentThreadId());
#else
    snprintf(cur_thread_info, MAX_PID_LEN, "tid:%04ld", pthread_self());
#endif

    return cur_thread_info;
}

static uint64_t log_get_timestamp()
{
    uint64_t ret = 0;
#ifdef WIN32
    struct timeb raw_time;
    ftime(&raw_time);
    ret = raw_time.time * 1000 + raw_time.millitm;
#else
    time_t t;
    t = time(NULL);
    ret = time(&t);
#endif
    return ret;
}

static bool log_lock()
{
#ifdef WIN32
    return true;
#else
    return !pthread_mutex_lock(&g_mutex);
#endif
}

static bool log_unlock()
{
#ifdef WIN32
    return true;
#else
    return !pthread_mutex_unlock(&g_mutex);
#endif
}

static int log_console_output(const char *log, size_t size)
{
    return printf("%.*s", (int)size, log);
}

#ifdef CONFIG_LOG_FILE_ENABLE
static int log_file_output(const char *log, size_t size)
{
    return fprintf(g_logfile, "%.*s", (int)size, log);
}
#endif

bool log_init(void)
{
#ifdef WIN32
#else
    if (pthread_mutex_init(&g_mutex, NULL)) {
        return false;
    }
#endif

#ifdef CONFIG_LOG_FILE_ENABLE
    if (g_logfile) {
        fclose(g_logfile);
    }

#ifdef WIN32
#else
    if (!log_lock()) {
        pthread_mutex_destroy(&g_mutex);
        return false;
    }
#endif
    char filename[MAX_FILENAME_LEN];
    snprintf(filename, MAX_FILENAME_LEN, "%s", CONFIG_LOG_FILE);
    g_logfile = fopen(filename, "a+");
    log_unlock();
#endif
    g_log_initialized = true;
    return true;
}

bool log_deinit(void)
{
    g_log_initialized = false;
#ifdef CONFIG_LOG_FILE_ENABLE
    if (g_logfile) {
        fflush(g_logfile);
        fclose(g_logfile);
    }
    g_logfile = NULL;
#endif
#ifdef WIN32
    return true;
#else
    return !pthread_mutex_destroy(&g_mutex);
#endif
}

static int32_t log_raw(LogLevel level, const char *tag, const char *file, const char *func,
                   const long line, const char *format, va_list args)
{
    int32_t fmt_result = -1;
    int32_t log_len = 0;
    char buffer[CONFIG_MAX_BUFFER_SIZE] = { 0 };
#ifdef CONFIG_LOG_FILE_ENABLE
    char *log_file_buffer = NULL;
    int32_t log_file_len = 0;
#endif

    if (!g_log_initialized) {
        log_init();
    }

#ifdef CONFIG_LOG_COLOR_ENABLE
    #ifdef WIN32
    WORD color[] = {
            FOREGROUND_RED | BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED,
            FOREGROUND_RED,
            FOREGROUND_GREEN | FOREGROUND_RED,
            FOREGROUND_GREEN,
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
            BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE
    };
    WORD wAttributes = color[level];
    HANDLE hConsole;
    CONSOLE_SCREEN_BUFFER_INFO info;
#else
    #define CSI_END "\033[0m"
    char *color[] = {
            "\033[31m\033[47m",
            "\033[31m",
            "\033[33m",
            "\033[32m",
            "\033[37m",
            "\033[30m\033[47m"
    };
#endif
#endif

    if (!log_lock()) {
        return fmt_result;
    }

#ifdef CONFIG_LOG_COLOR_ENABLE
    #ifdef WIN32
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &info);
    SetConsoleTextAttribute(hConsole, wAttributes);
#else
    fmt_result = sprintf(buffer + log_len, "%s", color[level]);
    log_len += fmt_result;
#endif
#endif

    fmt_result = snprintf(buffer + log_len, sizeof(buffer) - log_len, "%c/%s [%s][%s][%s(%ld):%s] ",
                          g_level[level].tag, g_level[level].level, log_get_time(), tag, file, line, func);

#ifdef CONFIG_LOG_FILE_ENABLE
    log_file_buffer = buffer + log_len;
    log_file_len = log_len;
#endif

    log_len += fmt_result;
    if (fmt_result < 0 || log_len >= CONFIG_MAX_BUFFER_SIZE) {
        goto end;
    }
    fmt_result = vsnprintf(buffer + log_len, sizeof(buffer) - log_len, format, args);
    log_len += fmt_result;
    if (fmt_result < 0 || log_len >= CONFIG_MAX_BUFFER_SIZE) {
        goto end;
    }

#ifdef CONFIG_LOG_FILE_ENABLE
    log_file_len = log_len - log_file_len;
#endif

#ifdef CONFIG_LOG_COLOR_ENABLE
    #ifndef WIN32
    fmt_result = sprintf(buffer + log_len, CSI_END);
    log_len += fmt_result;
#endif
#endif
    log_console_output(buffer, log_len);

#ifdef CONFIG_LOG_COLOR_ENABLE
    #ifdef WIN32
    SetConsoleTextAttribute(hConsole, info.wAttributes);
#endif
#endif

#ifdef CONFIG_LOG_FILE_ENABLE
    log_file_output(log_file_buffer, log_file_len);
#endif

end:
    log_unlock();
    return log_len;
}

int log_output(LogLevel level, const char *tag, const char *file, const char *func,
               const long line, const char *format, ...)
{
    int fmt_result = -1;
    va_list args;
    va_start(args, format);
    fmt_result = log_raw(level, tag, file, func, line, format, args);
    va_end(args);
    return fmt_result;
}
