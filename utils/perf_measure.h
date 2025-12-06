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

static inline void perf_log_elapsed(const char *label, double start_sec,
                                    double end_sec) {
  double sec = end_sec - start_sec;
  if (sec < 0)
    sec = 0.0;

  const char *unit = "ms";
  double val = sec * 1e3;
  int decimals = 4;

  if (sec < 1e-6) {
    // < 1 microsecond -> nanoseconds, integer
    val = sec * 1e9;
    unit = "ns";
    printf("[PERF] %s: %.0f%s\n", label, val, unit);
    return;
  } else if (sec < 1e-3) {
    // [1ns, 1ms) -> microseconds
    val = sec * 1e6;
    unit = "us";
    decimals = (val < 10.0) ? 1 : 0;
  } else {
    // >= 1ms -> milliseconds
    val = sec * 1e3;
    unit = "ms";
    decimals = (val < 10.0) ? 4 : (val < 100.0 ? 3 : 2); // e.g., 1.9159ms
  }

  char fmt[32];
  snprintf(fmt, sizeof(fmt), "[PERF] %%s: %%.%df%%s\n", decimals);
  printf(fmt, label, val, unit);
}

#if PERF_ENABLED

#define PerfMeasureLoop                                                        \
  for (                                                                        \
      struct {                                                                 \
        int done;                                                              \
        double start;                                                          \
      } _perf_ = {0, perf_now_seconds()};                                      \
      !_perf_.done; _perf_.done = 1,                                           \
        perf_log_elapsed("PerfMeasureLoop", _perf_.start, perf_now_seconds()))

#define PerfMeasureLoopNamed(label)                                            \
  for (                                                                        \
      struct {                                                                 \
        int done;                                                              \
        double start;                                                          \
        const char *lbl;                                                       \
      } _perf_ = {0, perf_now_seconds(), (label)};                             \
      !_perf_.done; _perf_.done = 1,                                           \
        perf_log_elapsed(_perf_.lbl, _perf_.start, perf_now_seconds()))

#else /* PERF_ENABLED == 0: no measurement, no logging */

#define PerfMeasureLoop for (int _perf_once_ = 0; !_perf_once_; _perf_once_ = 1)

#define PerfMeasureLoopNamed(label)                                            \
  for (int _perf_once_ = 0; !_perf_once_; _perf_once_ = 1)

#endif /* PERF_ENABLED */

#endif /* PERF_MEASURE_H */
