#ifndef LOG_H
#define LOG_H

#define debug(fmt, ...) log_debug_real(__func__, fmt, ##__VA_ARGS__)

void warn(const char *fmt, ...);
void log_debug_real(const char *func, const char *fmt, ...);

#endif
