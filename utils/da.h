#ifndef DA_H_INCLUDED
#define DA_H_INCLUDED

#include <limits.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  size_t len;
  size_t cap;
} DA_Header;

typedef struct DA_HeaderAligned {
  DA_Header h;
  max_align_t _align;
} DA_HeaderAligned;

#define DAH_OFFSET ((size_t)offsetof(DA_HeaderAligned, _align))

static inline DA_Header *da__hdr(const void *da) {
  return &((DA_HeaderAligned *)((unsigned char *)da - DAH_OFFSET))->h;
}

/* Overflow check: elem_size * cap + header must fit in size_t */
static inline int da__size_ok(const size_t elem_size, const size_t cap) {
  if (cap == 0)
    return 1;
  return elem_size <= (SIZE_MAX - DAH_OFFSET) / cap;
}

static inline void *da_new(size_t elem_size, size_t cap) {
  if (!da__size_ok(elem_size, cap)) {
    return NULL;
  }

  DA_HeaderAligned *hdr =
      (DA_HeaderAligned *)malloc(elem_size * cap + DAH_OFFSET);
  if (!hdr) {
    return NULL;
  }

  hdr->h.len = 0;
  hdr->h.cap = cap;
  return (unsigned char *)hdr + DAH_OFFSET;
}

static inline void *da__resize(void *da, size_t elem_size, size_t new_cap) {
  if (!da__size_ok(elem_size, new_cap)) {
    return NULL;
  }

  DA_HeaderAligned *hdr =
      (DA_HeaderAligned *)((unsigned char *)da - DAH_OFFSET);
  hdr = (DA_HeaderAligned *)realloc(hdr, elem_size * new_cap + DAH_OFFSET);
  if (!hdr) {
    return NULL;
  }

  hdr->h.cap = new_cap;
  return (unsigned char *)hdr + DAH_OFFSET;
}

static inline void *da__grow(void *da, size_t elem_size, size_t add) {
  if (!da) {
    size_t init_cap = (add > 16) ? add : 16;
    return da_new(elem_size, init_cap);
  }

  DA_Header *hdr = da__hdr(da);
  size_t needed = hdr->len + add;
  if (needed <= hdr->cap) {
    return da;
  }

  size_t new_cap = hdr->cap ? hdr->cap : 16;
  while (new_cap < needed) {
    if (new_cap > SIZE_MAX / 2) { /* avoid overflow */
      new_cap = needed;
      break;
    }
    new_cap *= 2;
  }

  return da__resize(da, elem_size, new_cap);
}

static inline void *da_append_raw(void *da, size_t elem_size,
                                  const void *elem) {
  da = da__grow(da, elem_size, 1);
  if (!da) {
    return NULL;
  }

  DA_Header *hdr = da__hdr(da);
  unsigned char *dest = (unsigned char *)da + elem_size * hdr->len;
  memcpy(dest, elem, elem_size);
  hdr->len++;
  return da;
}

static inline void *da_reserve(void *da, size_t elem_size, size_t min_cap) {
  size_t cap = da ? da__hdr(da)->cap : 0;
  if (cap >= min_cap) {
    return da;
  }
  if (!da) {
    return da_new(elem_size, min_cap);
  }
  return da__resize(da, elem_size, min_cap);
}

static inline size_t da_len(const void *da) { return da ? da__hdr(da)->len : 0; }

static inline size_t da_cap(const void *da) { return da ? da__hdr(da)->cap : 0; }

static inline void da_free(void *da) {
  if (da) {
    free((unsigned char *)da - DAH_OFFSET);
  }
}

#define make(T, cap) ((T *)da_new(sizeof(T), (cap)))

#define append(da, value)                                                      \
  do {                                                                         \
    typeof(da) _da = (da);                                                     \
    _da = da__grow(_da, sizeof(*_da), 1);                                      \
    if (_da) {                                                                 \
      DA_Header *_hdr = da__hdr(_da);                                          \
      _da[_hdr->len++] = (typeof(*_da))(value);                                \
    }                                                                          \
    (da) = _da;                                                                \
  } while (0)

#define foreach(it, da)                                                        \
  for (typeof(da) _da = (da), it = _da; _da && it < _da + da_len(_da); ++it)

#define remove_at(da, index)                                                   \
  do {                                                                         \
    typeof(da) _da = (da);                                                     \
    if (_da) {                                                                 \
      DA_Header *_hdr = da__hdr(_da);                                          \
      size_t _idx = (size_t)(index);                                           \
      if (_idx < _hdr->len) {                                                  \
        size_t _tail = _hdr->len - _idx - 1;                                   \
        if (_tail > 0) {                                                       \
          memmove(&_da[_idx], &_da[_idx + 1], _tail * sizeof(*_da));           \
        }                                                                      \
        _hdr->len--;                                                           \
      }                                                                        \
    }                                                                          \
    (da) = _da;                                                                \
  } while (0)

#endif /* DA_H_INCLUDED */
