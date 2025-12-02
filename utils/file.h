#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <io.h>
#include <share.h> /* for _SH_DENYNO */

#ifndef O_BINARY
#define O_BINARY _O_BINARY
#endif

/* Use _sopen_s on MSVC to avoid deprecation of _open */
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

/* Read the entire file into a malloc'ed buffer.
 * On success:
 *   *out_buf  -> allocated buffer (must be free()'d by caller)
 *   *out_size -> number of bytes read
 *   return 0
 * On failure: returns -1 and leaves *out_buf = NULL, *out_size = 0.
 */
int read_entire_file(const char *path, unsigned char **out_buf,
                     size_t *out_size) {
  if (!path || !out_buf || !out_size) {
    return -1;
  }

  *out_buf = NULL;
  *out_size = 0;

  int fd = FU_OPEN(path, FU_FLAGS);
  if (fd < 0) {
    return -1;
  }

  struct stat st;
  if (fstat(fd, &st) != 0) {
    FU_CLOSE(fd);
    return -1;
  }

  size_t size = (size_t)st.st_size;
  if (size == 0) {
    FU_CLOSE(fd);
    return 0;
  }

  unsigned char *buf = (unsigned char *)malloc(size);
  if (!buf) {
    FU_CLOSE(fd);
    return -1;
  }

  size_t total = 0;
  while (total < size) {
    int n = FU_READ(fd, buf + total, (unsigned int)(size - total));
    if (n < 0) {
      free(buf);
      FU_CLOSE(fd);
      return -1;
    }
    if (n == 0) {
      break; /* EOF */
    }
    total += (size_t)n;
  }

  FU_CLOSE(fd);

  *out_buf = buf;
  *out_size = total;
  return 0;
}

#endif /* FILE_UTILS_H */
