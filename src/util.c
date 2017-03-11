#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <Windows.h>
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#else
#include <unistd.h>
#endif

void error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  fputs("\n", stderr);
  va_end(args);
  exit(EXIT_FAILURE);
}

void lovrSleep(double seconds) {
#ifdef _WIN32
  Sleep((unsigned int)(seconds * 1000));
#else
  usleep((unsigned int)(seconds * 1000000));
#endif
}

void* lovrAlloc(size_t size, void (*destructor)(const Ref* ref)) {
  void* object = malloc(size);
  if (!object) return NULL;
  *((Ref*) object) = (Ref) { destructor, 1 };
  return object;
}

void lovrRetain(const Ref* ref) {
  ((Ref*) ref)->count++;
}

void lovrRelease(const Ref* ref) {
  if (--((Ref*) ref)->count == 0 && ref->free) ref->free(ref);
}

// https://github.com/starwing/luautf8
size_t utf8_decode(const char *s, const char *e, unsigned *pch) {
  unsigned ch;

  if (s >= e) {
    *pch = 0;
    return 0;
  }

  ch = (unsigned char)s[0];
  if (ch < 0xC0) goto fallback;
  if (ch < 0xE0) {
    if (s+1 >= e || (s[1] & 0xC0) != 0x80)
      goto fallback;
    *pch = ((ch   & 0x1F) << 6) |
            (s[1] & 0x3F);
    return 2;
  }
  if (ch < 0xF0) {
    if (s+2 >= e || (s[1] & 0xC0) != 0x80
                 || (s[2] & 0xC0) != 0x80)
      goto fallback;
    *pch = ((ch   & 0x0F) << 12) |
           ((s[1] & 0x3F) <<  6) |
            (s[2] & 0x3F);
    return 3;
  }
  {
    int count = 0; /* to count number of continuation bytes */
    unsigned res = 0;
    while ((ch & 0x40) != 0) { /* still have continuation bytes? */
      int cc = (unsigned char)s[++count];
      if ((cc & 0xC0) != 0x80) /* not a continuation byte? */
        goto fallback; /* invalid byte sequence, fallback */
      res = (res << 6) | (cc & 0x3F); /* add lower 6 bits from cont. byte */
      ch <<= 1; /* to test next bit */
    }
    if (count > 5)
      goto fallback; /* invalid byte sequence */
    res |= ((ch & 0x7F) << (count * 5)); /* add first byte */
    *pch = res;
    return count+1;
  }

fallback:
  *pch = ch;
  return 1;
}

int mkdir_p(const char* path) {
  char tmp[LOVR_PATH_MAX];
  strncpy(tmp, path, LOVR_PATH_MAX);
  for (char* p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
#if _WIN32
      CreateDirectory(tmp, NULL);
#else
      mkdir(tmp, 0700);
#endif
      *p = '/';
    }
  }

  mkdir(path, 0700);
  struct stat st;
  return stat(path, &st) || !S_ISDIR(st.st_mode);
}

void path_join(char* dest, const char* p1, const char* p2) {
  snprintf(dest, LOVR_PATH_MAX, "%s/%s", p1, p2);
}

void path_normalize(char* path) {
  char* p = strchr(path, '\\');
  while (p) {
    *p = '/';
    p = strchr(p + 1, '\\');
  }
}
