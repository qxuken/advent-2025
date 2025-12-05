#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../utils/da.h"
#include "../utils/file.h"
#include "../utils/log.h"

typedef struct {
  unsigned char *value;
  int size;
} Map;

static inline int index_2d(int stride, int x, int y) { return y * stride + x; }

static int dfs(Map *m, int x, int y) {
  const int size = m->size;
  const int stride = size + 1;
  unsigned char *restrict grid = m->value;

  const int y_start = (x >= 0 && y > 0) ? y - 1 : 0;
  const int y_end = (y + 1 < size) ? y + 1 : (size - 1);
  const int x_start = (x > 0) ? x - 1 : 0;
  const int x_end = (x + 1 < size) ? x + 1 : (size - 1);

  int neighbor_count = 0;
  for (int cy = y_start; cy <= y_end; ++cy) {
    unsigned char *row = grid + cy * stride + x_start;
    for (int cx = x_start; cx <= x_end; ++cx, ++row) {
      if (cy == y && cx == x) {
        continue;
      }
      if (*row == '@') {
        if (++neighbor_count > 3) {
          return 0;
        }
      }
    }
  }

  grid[index_2d(stride, x, y)] = 'x';

  int result = 1;
  for (int cy = y_start; cy <= y_end; ++cy) {
    unsigned char *row = grid + cy * stride + x_start;
    for (int cx = x_start; cx <= x_end; ++cx, ++row) {
      if (*row == '@') {
        result += dfs(m, cx, cy);
      }
    }
  }

  return result;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <file_input>\n", argv[0]);
    return 1;
  }

  unsigned char *data = NULL;
  if (!read_entire_file(argv[1], &data)) {
    fprintf(stderr, "Error opening file\n");
    return 1;
  }

  Map m;
  m.value = data;
  m.size = 0;

  for (size_t i = 0; i < da_len(data); ++i) {
    unsigned char c = data[i];
    if (c == '\r') {
      fprintf(stderr, "Remove CR from data\n");
      da_free(data);
      return 1;
    }
    if (c == '\n') {
      break;
    }
    m.size += 1;
  }

  log("m.size = %d\n", m.size);

  const int stride = m.size + 1;
  int counter = 0;

  for (int y = 0; y < m.size; ++y) {
    unsigned char *row = m.value + y * stride;
    for (int x = 0; x < m.size; ++x) {
      if (row[x] == '@') {
        counter += dfs(&m, x, y);
      }
    }
  }

#if LOGGER_ENABLED
  fwrite(m.value, 1, data_size, stdout);
  putchar('\n');
#endif

  print_value(counter, "%d");
  da_free(data);
  return 0;
}
