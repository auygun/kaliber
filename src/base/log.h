#ifndef LOG_H
#define LOG_H

#if defined(__ANDROID__)
#include <android/log.h>
#define LOG(...) __android_log_print(ANDROID_LOG_ERROR, "gltest", __VA_ARGS__)
#else
#include <stdarg.h>
#include <stdio.h>
#define LOG(...) printf(__VA_ARGS__)
#endif

#endif  // LOG_H
