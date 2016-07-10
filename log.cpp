#include <stdarg.h>
#include <stdio.h>


void log(const char * fmt, ...)
{
    va_list vargs;
    va_start(vargs, fmt);
    vfprintf(stdout, fmt, vargs);
    va_end(vargs);
    fputc('\n', stdout);
}
