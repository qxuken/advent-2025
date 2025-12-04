#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "defer.h"

#ifdef _WIN32
#include <io.h>
#include <share.h>

#ifndef O_BINARY
#define O_BINARY _O_BINARY
#endif

#ifdef _MSC_VER
static inline int fu_open_compat(const char *path, int oflag) {
  int fd = -1;
  /* read-only file, share with everyone, read-only permission flag */
  if (_sopen_s(&fd, path, oflag, _SH_DENYNO, _S_IREAD) != 0) {
    return -1;
  }
  return fd;
}
#define FU_OPEN(path, flags) fu_open_compat((path), (flags))
#else
#define FU_OPEN _open
#endif

#define FU_READ _read
#define FU_CLOSE _close
#define FU_FLAGS (O_RDONLY | O_BINARY)

#else /* non-Windows */

#include <unistd.h>
#define FU_OPEN open
#define FU_READ read
#define FU_CLOSE close
#define FU_FLAGS O_RDONLY

#endif

int read_entire_file(const char *path, unsigned char **out_buf,
                     size_t *out_size) {
  if (!path || !out_buf || !out_size) {
    return 0;
  }

  *out_buf = NULL;
  *out_size = 0;
  int fd;
  int ok = 1;
  DeferLoop(fd = FU_OPEN(path, FU_FLAGS), FU_CLOSE(fd)) {
    if (fd < 0) {
      return 0;
    }

    struct stat st;
    if (fstat(fd, &st) != 0) {
      ok = 0;
      continue;
    }
    size_t size = (size_t)st.st_size;
    if (size == 0) {
      ok = 0;
      continue;
    }

    unsigned char *buf = (unsigned char *)malloc(size);
    if (!buf) {
      ok = 0;
      continue;
    }
    *out_buf = buf;

    size_t total = 0;
    while (total < size && ok) {
      int n = FU_READ(fd, buf + total, (unsigned int)(size - total));
      if (n < 0) {
        *out_buf = NULL;
        free(buf);
        ok = 0;
        break;
      }
      if (n == 0) {
        break; /* EOF */
      }
      total += (size_t)n;
    }
    *out_size = total;
  }

  return ok;
}

#endif /* FILE_UTILS_H */
