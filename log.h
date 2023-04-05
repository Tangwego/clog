#ifndef LOG_H_
#define LOG_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TLOG_ASSERT = 0x0,
    TLOG_ERROR,
    TLOG_WARN,
    TLOG_INFO,
    TLOG_DEBUG,
    TLOG_VERBOSE
}LogLevel;

bool log_init(void);

bool log_deinit(void);

int log_output(LogLevel level, const char *tag, const char *file, const char *func,
               const long line, const char *format, ...);

#define loga(tag, format, ...) log_output(TLOG_ASSERT,  tag, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define loge(tag, format, ...) log_output(TLOG_ERROR,   tag, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define logw(tag, format, ...) log_output(TLOG_WARN,    tag, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define logi(tag, format, ...) log_output(TLOG_INFO,    tag, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define logd(tag, format, ...) log_output(TLOG_DEBUG,   tag, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define logv(tag, format, ...) log_output(TLOG_VERBOSE, tag, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif