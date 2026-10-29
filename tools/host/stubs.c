/* engine error reporting for the host tools: printed to stderr */
#include <stdio.h>
#include <stdarg.h>
void tts_set_errorf(int code, const char *fmt, ...) { va_list a; va_start(a, fmt); fprintf(stderr, "error %d: ", code); vfprintf(stderr, fmt, a); fputc('\n', stderr); va_end(a); }
void tts_set_error(int code) { fprintf(stderr, "error %d\n", code); }
void tts_set_error_contextf(int code, const char *ctx, const char *fmt, ...) { (void)ctx; fprintf(stderr, "error %d: %s\n", code, fmt); }
void tts_set_error_errno(int code, const char *s) { fprintf(stderr, "error %d: %s\n", code, s); }
