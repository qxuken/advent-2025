#ifndef PERF_MEASURE_H
#define PERF_MEASURE_H

#include <stdint.h>

#include "debug.h"
#include "log.h"

#ifndef PERF_ENABLED
#define PERF_ENABLED DEBUG
#endif

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#elif defined(__APPLE__)

#include <mach/mach_time.h>

#else /* POSIX or fallback */

#include <time.h>

#endif

static inline double perf_now_seconds(void) {
#if defined(_WIN32)

  static LARGE_INTEGER perf_freq;
  static int perf_freq_init = 0;

  if (!perf_freq_init) {
    QueryPerformanceFrequency(&perf_freq);
    perf_freq_init = 1;
  }

  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return (double)counter.QuadPart / (double)perf_freq.QuadPart;

#elif defined(__APPLE__)

  static mach_timebase_info_data_t perf_timebase;
  static int perf_timebase_init = 0;

  if (!perf_timebase_init) {
    mach_timebase_info(&perf_timebase);
    perf_timebase_init = 1;
  }

  uint64_t t = mach_absolute_time();
  double ns =
      (double)t * (double)perf_timebase.numer / (double)perf_timebase.denom;
  return ns * 1e-9;

#else /* POSIX or fallback */

  struct timespec ts;
#if defined(CLOCK_MONOTONIC)
  clock_gettime(CLOCK_MONOTONIC, &ts);
#else
  clock_gettime(CLOCK_REALTIME, &ts);
#endif
  return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;

#endif
}

static inline void perf_log_elapsed(const char *label, const char *file,
                                    int line, const char *func,
                                    double start_sec, double end_sec) {
  double ms = (end_sec - start_sec) * 1000.0;
  printf("[PERF] %s (%s:%d, %s): %.6f ms\n", label, file, line, func, ms);
}

#if PERF_ENABLED

#define PerfMeasureLoop                                                        \
  for (                                                                        \
      struct {                                                                 \
        int done;                                                              \
        double start;                                                          \
      } _perf_ = {0, perf_now_seconds()};                                      \
      !_perf_.done; _perf_.done = 1,                                           \
        perf_log_elapsed("PerfMeasureLoop", __FILE__, __LINE__, __func__,      \
                         _perf_.start, perf_now_seconds()))

#define PerfMeasureLoopNamed(label)                                            \
  for (                                                                        \
      struct {                                                                 \
        int done;                                                              \
        double start;                                                          \
        const char *lbl;                                                       \
      } _perf_ = {0, perf_now_seconds(), (label)};                             \
      !_perf_.done; _perf_.done = 1,                                           \
        perf_log_elapsed(_perf_.lbl, __FILE__, __LINE__, __func__,             \
                         _perf_.start, perf_now_seconds()))

#else /* PERF_ENABLED == 0: no measurement, no logging */

#define PerfMeasureLoop for (int _perf_once_ = 0; !_perf_once_; _perf_once_ = 1)

#define PerfMeasureLoopNamed(label)                                            \
  for (int _perf_once_ = 0; !_perf_once_; _perf_once_ = 1)

#endif /* PERF_ENABLED */

#endif /* PERF_MEASURE_H */
