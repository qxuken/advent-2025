#ifndef LOG_UTILS_H
#define LOG_UTILS_H

#include <stdio.h>

#include "debug.h"

#ifndef LOGGER_ENABLED
#define LOGGER_ENABLED DEBUG
#endif

#define log(...)                                                               \
  do {                                                                         \
    if (LOGGER_ENABLED)                                                        \
      printf(__VA_ARGS__);                                                     \
  } while (0)

#define print_value(val, fmt) printf("%s = " fmt "\n", #val, (val))

#define log_value(val, fmt)                                                    \
  do {                                                                         \
    if (LOGGER_ENABLED)                                                        \
      print_value(val, fmt);                                                   \
  } while (0)

#endif /* LOG_UTILS_H */
