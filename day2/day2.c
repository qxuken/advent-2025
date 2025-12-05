#include <math.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "../utils/da.h"
#include "../utils/file.h"
#include "../utils/log.h"
#include "../utils/threadpool.h"
#include "../utils/numbers.h"

int any_parts_repeating(uint64_t n) {
  char buf[21];
  int len = u64_to_str(n, buf);

  for (int pat_len = 1; pat_len <= len / 2; ++pat_len) {
    if (len % pat_len != 0)
      continue; // can't be made of equal-sized blocks

    int ok = 1;
    int k = 0; // index in pattern [0..pat_len-1]
    for (int i = pat_len; i < len; ++i) {
      if (buf[i] != buf[k]) {
        ok = 0;
        break;
      }
      if (++k == pat_len)
        k = 0;
    }
    if (ok)
      return 1;
  }
  return 0;
}

// NOTE: There was a bug, that could be caught with -Wall -Wextra -Wconversion
// -Wsign-conversion
uint64_t sum_patterns(uint64_t from, uint64_t to) {
  log("%llu..=%llu\n", from, to);
  uint64_t sum = 0;
  for (uint64_t i = from; i <= to; i++) {
    if (any_parts_repeating(i)) {
      sum += i;
    }
  }
  log_value(sum, "%llu");
  return sum;
}

typedef struct {
  uint64_t from;
  uint64_t to;
  _Atomic uint64_t *global_sum;
} sum_job_t;

static void sum_patterns_job(void *arg) {
  sum_job_t *job = (sum_job_t *)arg;

  uint64_t local = sum_patterns(job->from, job->to);

  /* Atomically add local result to global total */
  atomic_fetch_add_explicit(job->global_sum, local, memory_order_relaxed);

  free(job); /* job was allocated in main */
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <file_input>\n", argv[0]);
    return EXIT_FAILURE;
  }

  unsigned char *data = NULL;
  if (!read_entire_file(argv[1], &data)) {
    perror("Error reading file");
    return EXIT_FAILURE;
  }
  log("Read %zu bytes\n", da_len(data));

  threadpool_t pool;
  if (threadpool_init(&pool, 0) != 0) {
    da_free(data);
    fprintf(stderr, "Failed to initialize thread pool\n");
    return EXIT_FAILURE;
  }

  unsigned char *p = data;
  unsigned char *end = data + da_len(data);

  _Atomic uint64_t total_sum = 0;

  while (p < end) {
    uint64_t from = 0;
    if (!parse_next_number(&p, end, &from)) {
      break;
    }
    uint64_t to = 0;
    if (!parse_next_number(&p, end, &to)) {
      break;
    }

    sum_job_t *job = (sum_job_t *)malloc(sizeof(*job));
    if (!job) {
      fprintf(stderr, "Out of memory allocating job\n");
      da_free(data);
      threadpool_destroy(&pool);
      return EXIT_FAILURE;
    }
    job->from = from;
    job->to = to;
    job->global_sum = &total_sum;

    int rc = threadpool_submit(&pool, sum_patterns_job, job);
    if (rc != 0) {
      fprintf(stderr, "threadpool_submit failed (code=%d)\n", rc);
      free(job);
      da_free(data);
      threadpool_destroy(&pool);
      return EXIT_FAILURE;
    }
  }

  threadpool_destroy(&pool);
  uint64_t final_sum = atomic_load_explicit(&total_sum, memory_order_relaxed);
  print_value(final_sum, "%llu");

  da_free(data);
  return 0;
}
