#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../utils/da.h"
#include "../utils/file.h"
#include "../utils/numbers.h"
#include "../utils/perf_measure.h"

typedef struct {
  uint64_t left;
  uint64_t right;
} RangeInclusive;

int range_compare(const void *a, const void *b) {
  const RangeInclusive *ra = a;
  const RangeInclusive *rb = b;

  if (ra->left < rb->left)
    return -1;
  if (ra->left > rb->left)
    return 1;
  return 0;
}

int range_overlap(const RangeInclusive *a, const RangeInclusive *b) {
  return a->left <= b->right && b->left <= a->right;
}

void range_merge_left(RangeInclusive *a, const RangeInclusive *b) {
  if (b->left < a->left)
    a->left = b->left;
  if (b->right > a->right)
    a->right = b->right;
}

void merge_ranges(RangeInclusive *ranges) {
  size_t n = da_len(ranges);
  qsort(ranges, n, sizeof(*ranges), range_compare);

  for (size_t i = 0; i + 1 < da_len(ranges); ++i) {
    if (range_overlap(&ranges[i], &ranges[i + 1])) {
      range_merge_left(&ranges[i], &ranges[i + 1]);
      remove_at(ranges, i + 1);
      i--;
    }
  }
}
int is_in_ranges(const RangeInclusive *ranges, const uint64_t number) {
  size_t len = da_len(ranges);
  size_t left = 0;
  size_t right = len;
  while (left < right) {
    const size_t mid = left + (right - left) / 2;
    const RangeInclusive *r = &ranges[mid];
    if (number < r->left) {
      right = mid;
    } else if (number > r->right) {
      left = mid + 1;
    } else {
      return 1;
    }
  }
  return 0;
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
        return 1;
      }
    }
    DeferLoopEnd(da_free(data)) {
      log("Read bytes %llu\n", da_len(data));
      unsigned char *p = data;
      unsigned char *end = p + da_len(data);
      RangeInclusive *ranges = make(RangeInclusive, 200);
      DeferLoopEnd(da_free(ranges)) {
        PerfMeasureLoopNamed("parsing") {
          log("Parsing\n");
          while (1) {
            if (*(p) == '\n' && *(p + 1) == '\n')
              break;
            RangeInclusive range = {0};
            if (!parse_row_number(&p, end, &range.left)) {
              fprintf(stderr, "Unexpected data\n");
              return 1;
            }
            if (!parse_row_number(&p, end, &range.right)) {
              fprintf(stderr, "Unexpected data\n");
              return 1;
            }
            log("%llu..%llu\n", range.left, range.right);
            append(ranges, range);
          }
        } // PerfMeasureLoopNamed("parsing")

        PerfMeasureLoopNamed("optimizing") {
          log("Optimizing\n");
          merge_ranges(ranges);
        }

#if DEBUG
        foreach (range, ranges) {
          log("%llu..%llu\n", range->left, range->right);
        }
#endif
        PerfMeasureLoopNamed("fresh_counter") {
          uint64_t counter = 0;
          while (1) {
            size_t number = 0;
            if (!parse_row_number(&p, end, &number)) {
              break;
            }
            int matched = is_in_ranges(ranges, number);
            counter += (uint64_t)matched;
            log("%llu (%d)\n", number, matched);
          }
          print_value(counter, "%llu");
        } // PerfMeasureLoopNamed("fresh_counter")
        PerfMeasureLoopNamed("range_counter") {
          uint64_t counter = 0;
          foreach (range, ranges) {
            uint64_t len = (range->right - range->left + 1);
            log_value(len, "%llu");
            counter += len;
          }
          print_value(counter, "%llu");
        } // PerfMeasureLoopNamed("range_counter")
      } // DeferLoopEnd(da_free(ranges))
    } // DeferLoopEnd
  } // PerfMeasureLoopNamed("entire")
  return EXIT_SUCCESS;
}
