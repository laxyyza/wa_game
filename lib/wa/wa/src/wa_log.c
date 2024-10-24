#include "wa_log.h"

#include <stdio.h>
#include <stdarg.h>

#define WA_LOG_LEN 4096

/* Terminal colors */
#ifdef _WA_WAYLAND
#define FG_RED "\033[91m"
#define BG_RED "\033[101m"
#define FG_GREEN "\033[92m"
#define FG_ORANGE "\033[93m"
#define FG_DEFAULT "\033[39m"
#define BG_DEFAULT "\033[49m"
#define FG_BLACK "\033[30m"

#define C_DEFAULT FG_DEFAULT BG_DEFAULT
#define C_FATAL BG_RED FG_BLACK

#elif _WA_WIN32

#define FG_RED ""
#define BG_RED ""
#define FG_GREEN "" 
#define FG_ORANGE ""
#define FG_DEFAULT ""
#define BG_DEFAULT ""
#define FG_BLACK ""

#define C_DEFAULT ""
#define C_FATAL ""

#endif // _WA_WIN32

static const char* wa_log_str[WA_LOG_LEVEL_LEN] = {
    "WA [" FG_RED "FATAL" FG_DEFAULT "]",
    "WA [" FG_RED "ERROR" FG_DEFAULT "]",
    "WA [" FG_ORANGE "WARN" FG_DEFAULT "]",
    "WA [" FG_GREEN "INFO" FG_DEFAULT "]",
    "WA [DEBUG]",
    "WA [VBOSE]",
};

static const char* wa_log_color[WA_LOG_LEVEL_LEN] = {
    C_FATAL,            /* WA_FATAL */
    FG_RED,             /* WA_ERROR */
    FG_DEFAULT,         /* WA_WARN */
    FG_DEFAULT,         /* WA_INFO */
    FG_DEFAULT,          /* WA_DEBUG */
    FG_DEFAULT          /* WA_VBOSE */
};

static enum wa_log_level wa_level = WA_DEBUG;

void wa_log_set_level(enum wa_log_level level)
{
    wa_level = level;
}

void wa_log_msg(enum wa_log_level level, const char* filename, i32 line, const char* format, ...)
{
    if (wa_level < level)
        return;

    char output[WA_LOG_LEN];
    FILE* file = (level <= WA_WARN) ? stderr : stdout;

    va_list args;
    va_start(args, format);

    vsprintf(output, format, args);

    va_end(args);

    if (filename)
        fprintf(file, "%s [%s:%d]:\t%s", wa_log_str[level], filename, line, wa_log_color[level]);
    else
        fprintf(file, "%s:\t%s", wa_log_str[level], wa_log_color[level]);

    fprintf(file, "%s%s%s", wa_log_color[level], output, C_DEFAULT);

    fflush(file);
}
