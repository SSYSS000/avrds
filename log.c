#include <stdarg.h>
#include <stdio.h>

void warn(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
}
