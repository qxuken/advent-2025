#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../utils/da.h"
#include "../utils/file.h"
#include "../utils/perf_measure.h"

int parse_row_number(unsigned char *p, unsigned char *end,
                     const size_t row_size, uint64_t *n) {
  while (p < end && !isdigit(*p))
    p += row_size;
  if (p >= end)
    return 0;
  unsigned char *q = p;
  while (q < end && isdigit(*q)) {
    q += row_size;
  }
  *n = 0;
  for (unsigned char *r = p; r < q; r += row_size)
    *n = *n * 10 + (*r - '0');
  return 1;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <file_input>\n", argv[0]);
    return 1;
  }

  unsigned char *data = NULL;

  PerfMeasureLoopNamed("entire") {
    PerfMeasureLoopNamed("read") {
      if (!read_entire_file(argv[1], &data)) {
        fprintf(stderr, "Error opening file\n");
        return EXIT_FAILURE;
      }
    }
    unsigned char *p = &data[0];
    unsigned char *end = &data[da_len(data) - 1];

    DeferLoopEnd(da_free(data)) {
      log("Read bytes %llu\n", da_len(data));

      size_t row_size = 0;

      PerfMeasureLoopNamed("row_size") {
        unsigned char *q = memchr(data, '\n', da_len(data));
        row_size = (size_t)(q - p) + 1;
      }

      uint64_t **numbers = make(uint64_t *, row_size);
      PerfMeasureLoopNamed("parse") {
        size_t i = 0;
        while (p < end && *p != '\n') {
          uint64_t num = 0;
          if (!parse_row_number(p, end, row_size, &num)) {
            i++;
            p++;
            continue;
          }
          if (i >= da_len(numbers)) {
            append(numbers, make(uint64_t, 10));
          }
          append(numbers[i], num);
          p++;
        }
      }
#if DEBUG
      foreach (row, numbers) {
        foreach (it, *row) {
          log("%llu ", *it);
        }
        log("\n");
      }
#endif
      log_value(da_cap(numbers), "%lld");
      log_value(da_len(numbers), "%lld");

      PerfMeasureLoopNamed("advance") {
        unsigned char *q = memchr(data, '\n', da_len(data));
        row_size = (size_t)(q - p) + 1;
        while (p < end && (*p != '*' && *p != '+'))
          p++;
      }

      uint64_t sum = 0;
      PerfMeasureLoopNamed("sum") {
        size_t i = 0;
        while (p < end) {
          uint64_t acc = 0;
          switch (*p) {
          case '*': {
            acc = 1;
            foreach (n, numbers[i]) {
              acc *= *n;
            }
          } break;
          case '+': {
            acc = 0;
            foreach (n, numbers[i]) {
              acc += *n;
            }
          } break;
          default: {
            fprintf(stderr, "Unexpected data at %llu(%d)\n", p - &data[0], *p);
            return EXIT_FAILURE;
          }
          }
          p++;
          sum += acc;
          while (*p == ' ')
            ++p;
          i++;
        }
      }
      print_value(sum, "%llu");

      foreach (row, numbers) {
        da_free(*row);
      }
      da_free(numbers);
    } // DeferLoopEnd(da_free(data))
  } // PerfMeasureLoopNamed("entire")
  return EXIT_SUCCESS;
}
