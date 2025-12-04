#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../utils/file.h"
#include "../utils/log.h"

typedef struct {
  unsigned char *value;
  int size;
} Map;

static inline int index_2d(int size, int x, int y) {
  return y * (size + 1) + x;
}

static int traverse_map_rec(Map *m, int x, int y);
static int traverse_map_around(Map *m, int x, int y);

static int traverse_map_around(Map *m, int x, int y) {
  int counter = 0;
  int size = m->size;

  int y_start = (y - 1 >= 0) ? (y - 1) : 0;
  int y_end = (y + 1 < size) ? (y + 1) : (size - 1);
  int x_start = (x - 1 >= 0) ? (x - 1) : 0;
  int x_end = (x + 1 < size) ? (x + 1) : (size - 1);

  for (int cy = y_start; cy <= y_end; ++cy) {
    for (int cx = x_start; cx <= x_end; ++cx) {
      int idx = index_2d(size, cx, cy);
      if (m->value[idx] == '@') {
        counter += traverse_map_rec(m, cx, cy);
      }
    }
  }

  return counter;
}

static int traverse_map_rec(Map *m, int x, int y) {
  int counter = 0;
  int size = m->size;

  int y_start = (y - 1 >= 0) ? (y - 1) : 0;
  int y_end = (y + 1 < size) ? (y + 1) : (size - 1);
  int x_start = (x - 1 >= 0) ? (x - 1) : 0;
  int x_end = (x + 1 < size) ? (x + 1) : (size - 1);

  for (int cy = y_start; cy <= y_end; ++cy) {
    for (int cx = x_start; cx <= x_end; ++cx) {
      if (cy == y && cx == x) {
        continue;
      }
      int idx = index_2d(size, cx, cy);
      if (m->value[idx] == '@') {
        counter += 1;
        if (counter > 3) {
          return 0;
        }
      }
    }
  }

  m->value[index_2d(size, x, y)] = 'x';
  return 1 + traverse_map_around(m, x, y);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <file_input>\n", argv[0]);
    return 1;
  }

  unsigned char *data = NULL;
  size_t data_size = 0;
  if (!read_entire_file(argv[1], &data, &data_size)) {
    fprintf(stderr, "Error opening file\n");
    return 1;
  }

  Map m;
  m.value = data;
  m.size = 0;

  for (size_t i = 0; i < data_size; ++i) {
    unsigned char c = data[i];
    if (c == '\r') {
      fprintf(stderr, "Remove CR from data\n");
      free(data);
      return 1;
    }
    if (c == '\n') {
      break;
    }
    m.size += 1;
  }

  log("m.size = %d\n", m.size);

  int counter = 0;
  log("========\n");
  for (int y = 0; y < m.size; ++y) {
    for (int x = 0; x < m.size; ++x) {
      int idx = index_2d(m.size, x, y);
      if (m.value[idx] == '@') {
        counter += traverse_map_rec(&m, x, y);
      }
    }
  }

#if LOGGER_ENABLED
  fwrite(m.value, 1, data_size, stdout);
  putchar('\n');
#endif

  print_value(counter, "%d");

  free(data);
  return 0;
}
