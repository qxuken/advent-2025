#ifndef THREADPOOL_UTILS_H
#define THREADPOOL_UTILS_H

#include <stddef.h>

#ifndef TP_MAX_THREADS
#define TP_MAX_THREADS 64
#endif

#ifndef TP_MAX_QUEUE
#define TP_MAX_QUEUE 256
#endif

typedef void (*tp_task_fn)(void *arg);

typedef struct {
  tp_task_fn func;
  void *arg;
} tp_task_t;

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#endif

#include "defer.h"

/* Cross-platform mutex helpers and scope guard */

#ifdef _WIN32
#define mutex_take(mtx) EnterCriticalSection((mtx))
#define mutex_drop(mtx) LeaveCriticalSection((mtx))
#else
static inline void mutex_take(pthread_mutex_t *mtx) { pthread_mutex_lock(mtx); }
static inline void mutex_drop(pthread_mutex_t *mtx) {
  pthread_mutex_unlock(mtx);
}
#endif

#define MutexScope(mutex_ptr)                                                  \
  DeferLoop(mutex_take(mutex_ptr), mutex_drop(mutex_ptr))

typedef struct threadpool {
#ifdef _WIN32
  HANDLE threads[TP_MAX_THREADS];
#else
  pthread_t threads[TP_MAX_THREADS];
#endif
  int nthreads;
  tp_task_t queue[TP_MAX_QUEUE];
  int head;
  int tail;
  int count;
  int stop;
#ifdef _WIN32
  CRITICAL_SECTION mutex;
  CONDITION_VARIABLE cond_nonempty;
  CONDITION_VARIABLE cond_nonfull;
#else
  pthread_mutex_t mutex;
  pthread_cond_t cond_nonempty;
  pthread_cond_t cond_nonfull;
#endif
} threadpool_t;

int threadpool_init(threadpool_t *pool, int nthreads);
int threadpool_submit(threadpool_t *pool, tp_task_fn func, void *arg);
void threadpool_destroy(threadpool_t *pool);

static int tp_get_cpu_count(void) {
  int n = 1;
#ifdef _WIN32
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  n = (int)info.dwNumberOfProcessors;
  if (n < 1)
    n = 1;
#else
#ifdef _SC_NPROCESSORS_ONLN
  long ln = sysconf(_SC_NPROCESSORS_ONLN);
  if (ln < 1)
    ln = 1;
  n = (int)ln;
#else
  n = 1;
#endif
#endif
  return n;
}

#ifdef _WIN32

static DWORD WINAPI tp_worker_main(LPVOID arg) {
  threadpool_t *pool = (threadpool_t *)arg;

  for (;;) {
    tp_task_t task;
    int should_exit = 0;

    MutexScope(&pool->mutex) {
      while (pool->count == 0 && !pool->stop) {
        SleepConditionVariableCS(&pool->cond_nonempty, &pool->mutex, INFINITE);
      }

      if (pool->stop && pool->count == 0) {
        should_exit = 1;
        /* continue exits the DeferLoop, running mutex_drop */
        continue;
      }

      task = pool->queue[pool->head];
      pool->head = (pool->head + 1) % TP_MAX_QUEUE;
      pool->count--;
      WakeConditionVariable(&pool->cond_nonfull);
    }

    if (should_exit) {
      break; /* exit thread */
    }

    task.func(task.arg);
  }

  return 0;
}

#else /* !_WIN32 */

static void *tp_worker_main(void *arg) {
  threadpool_t *pool = (threadpool_t *)arg;

  for (;;) {
    tp_task_t task;
    int should_exit = 0;

    MutexScope(&pool->mutex) {
      while (pool->count == 0 && !pool->stop) {
        pthread_cond_wait(&pool->cond_nonempty, &pool->mutex);
      }

      if (pool->stop && pool->count == 0) {
        should_exit = 1;
        /* continue exits the DeferLoop, running mutex_drop */
        continue;
      }

      task = pool->queue[pool->head];
      pool->head = (pool->head + 1) % TP_MAX_QUEUE;
      pool->count--;
      pthread_cond_signal(&pool->cond_nonfull);
    }

    if (should_exit) {
      break;
    }

    task.func(task.arg);
  }

  return NULL;
}

#endif /* _WIN32 */

int threadpool_init(threadpool_t *pool, int nthreads) {
  if (!pool) {
#ifdef _WIN32
    return ERROR_INVALID_PARAMETER;
#else
    return EINVAL;
#endif
  }

  if (nthreads <= 0) {
    nthreads = tp_get_cpu_count();
  }
  if (nthreads > TP_MAX_THREADS) {
    nthreads = TP_MAX_THREADS;
  }
  if (nthreads < 1) {
    nthreads = 1;
  }

  pool->nthreads = nthreads;
  pool->head = pool->tail = pool->count = 0;
  pool->stop = 0;

#ifdef _WIN32

  InitializeCriticalSection(&pool->mutex);
  InitializeConditionVariable(&pool->cond_nonempty);
  InitializeConditionVariable(&pool->cond_nonfull);

  for (int i = 0; i < nthreads; ++i) {
    HANDLE h = CreateThread(NULL, 0, tp_worker_main, pool, 0, NULL);
    if (h == NULL) {
      MutexScope(&pool->mutex) {
        pool->stop = 1;
        WakeAllConditionVariable(&pool->cond_nonempty);
        WakeAllConditionVariable(&pool->cond_nonfull);
      }

      for (int j = 0; j < i; ++j) {
        WaitForSingleObject(pool->threads[j], INFINITE);
        CloseHandle(pool->threads[j]);
      }

      DeleteCriticalSection(&pool->mutex);
      return (int)GetLastError();
    }
    pool->threads[i] = h;
  }

  return 0;

#else /* !_WIN32 */

  if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
    return errno ? errno : -1;
  }

  if (pthread_cond_init(&pool->cond_nonempty, NULL) != 0) {
    pthread_mutex_destroy(&pool->mutex);
    return errno ? errno : -1;
  }

  if (pthread_cond_init(&pool->cond_nonfull, NULL) != 0) {
    pthread_cond_destroy(&pool->cond_nonempty);
    pthread_mutex_destroy(&pool->mutex);
    return errno ? errno : -1;
  }

  for (int i = 0; i < nthreads; ++i) {
    int rc = pthread_create(&pool->threads[i], NULL, tp_worker_main, pool);
    if (rc != 0) {
      MutexScope(&pool->mutex) {
        pool->stop = 1;
        pthread_cond_broadcast(&pool->cond_nonempty);
        pthread_cond_broadcast(&pool->cond_nonfull);
      }

      for (int j = 0; j < i; ++j) {
        pthread_join(pool->threads[j], NULL);
      }

      pthread_cond_destroy(&pool->cond_nonfull);
      pthread_cond_destroy(&pool->cond_nonempty);
      pthread_mutex_destroy(&pool->mutex);
      return rc;
    }
  }

  return 0;

#endif /* _WIN32 */
}

int threadpool_submit(threadpool_t *pool, tp_task_fn func, void *arg) {
  if (!pool || !func) {
#ifdef _WIN32
    return ERROR_INVALID_PARAMETER;
#else
    return EINVAL;
#endif
  }

#ifdef _WIN32

  int rc = 0;

  MutexScope(&pool->mutex) {
    while (pool->count == TP_MAX_QUEUE && !pool->stop) {
      SleepConditionVariableCS(&pool->cond_nonfull, &pool->mutex, INFINITE);
    }

    if (pool->stop) {
      rc = ERROR_CANCELLED;
      /* continue exits the DeferLoop, running mutex_drop */
      continue;
    }

    pool->queue[pool->tail].func = func;
    pool->queue[pool->tail].arg = arg;
    pool->tail = (pool->tail + 1) % TP_MAX_QUEUE;
    pool->count++;

    WakeConditionVariable(&pool->cond_nonempty);
  }

  return rc;

#else /* !_WIN32 */

  int rc = 0;

  MutexScope(&pool->mutex) {
    while (pool->count == TP_MAX_QUEUE && !pool->stop) {
      pthread_cond_wait(&pool->cond_nonfull, &pool->mutex);
    }

    if (pool->stop) {
      rc = ECANCELED;
      /* continue exits the DeferLoop, running mutex_drop */
      continue;
    }

    pool->queue[pool->tail].func = func;
    pool->queue[pool->tail].arg = arg;
    pool->tail = (pool->tail + 1) % TP_MAX_QUEUE;
    pool->count++;

    pthread_cond_signal(&pool->cond_nonempty);
  }

  return rc;

#endif /* _WIN32 */
}

void threadpool_destroy(threadpool_t *pool) {
  if (!pool)
    return;

#ifdef _WIN32

  MutexScope(&pool->mutex) {
    pool->stop = 1;
    WakeAllConditionVariable(&pool->cond_nonempty);
    WakeAllConditionVariable(&pool->cond_nonfull);
  }

  for (int i = 0; i < pool->nthreads; ++i) {
    WaitForSingleObject(pool->threads[i], INFINITE);
    CloseHandle(pool->threads[i]);
  }

  DeleteCriticalSection(&pool->mutex);

#else /* !_WIN32 */

  MutexScope(&pool->mutex) {
    pool->stop = 1;
    pthread_cond_broadcast(&pool->cond_nonempty);
    pthread_cond_broadcast(&pool->cond_nonfull);
  }

  for (int i = 0; i < pool->nthreads; ++i) {
    pthread_join(pool->threads[i], NULL);
  }

  pthread_cond_destroy(&pool->cond_nonfull);
  pthread_cond_destroy(&pool->cond_nonempty);
  pthread_mutex_destroy(&pool->mutex);

#endif /* _WIN32 */
}

#endif /* THREADPOOL_UTILS_H */
