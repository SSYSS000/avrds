#include <stdarg.h>
#include <stdio.h>

void warn(const char *fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}

void log_debug_real(const char *func, const char *fmt, ...)
{
    va_list va;

    /* FIXME: in multithreaded env, printed text may not be consecutive */
    fprintf(stderr, "debug[%s]: ", func);
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}
