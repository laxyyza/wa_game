#ifndef _WA_LOG_H_
#define _WA_LOG_H_

#include "wa_int.h"

enum wa_log_level
{
    WA_FATAL,
    WA_ERROR,
    WA_WARN,
    WA_INFO,
    WA_DEBUG,
    WA_VBOSE,

    WA_LOG_LEVEL_LEN
};

void wa_log_set_level(enum wa_log_level level);
void wa_log_msg(enum wa_log_level level, const char* file, i32 line, const char* format, ...);

/*
 * wa_logf() will log the file name and line number with the message
 * wa_log() will just log a message
 */
#define wa_logf(level, format, ...) wa_log_msg(level, __FILE_NAME__, __LINE__, format, ##__VA_ARGS__)
#define wa_log(level, format, ...) wa_log_msg(level, NULL, 0, format, ##__VA_ARGS__)

#endif // _WA_LOG_H_
