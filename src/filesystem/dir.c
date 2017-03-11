#include "filesystem/filesystem.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#ifndef _WIN32
#include <unistd.h>
#endif

static int dirExists(Archive* archive, const char* path) {
  char fullpath[LOVR_PATH_MAX];
  path_join(fullpath, archive->path, path);
  return !access(fullpath, 0);
}

static void dirGetDirectoryItems(Archive* archive, const char* path, getDirectoryItemsCallback callback, void* userdata) {
  char fullpath[LOVR_PATH_MAX];
  path_join(fullpath, archive->path, path);

  DIR* dir;
  if ((dir = opendir(fullpath)) == NULL) {
    return;
  }

  struct dirent* item;
  while ((item = readdir(dir)) != NULL) {
    if (strcmp(item->d_name, ".") && strcmp(item->d_name, "..")) {
      callback(item->d_name, userdata);
    }
  }

  closedir(dir);
}

static int dirGetSize(Archive* archive, const char* path, size_t* size) {
  char fullpath[LOVR_PATH_MAX];
  path_join(fullpath, archive->path, path);

  struct stat st;
  if (stat(fullpath, &st)) {
    return 1;
  }

  *size = st.st_size;
  return 0;
}

static int dirIsDirectory(Archive* archive, const char* path) {
  char fullpath[LOVR_PATH_MAX];
  path_join(fullpath, archive->path, path);

  struct stat st;
  return !stat(fullpath, &st) && S_ISDIR(st.st_mode);
}

static int dirIsFile(Archive* archive, const char* path) {
  char fullpath[LOVR_PATH_MAX];
  path_join(fullpath, archive->path, path);

  struct stat st;
  return !stat(fullpath, &st) && S_ISREG(st.st_mode);
}

static int dirLastModified(Archive* archive, const char* path) {
  char fullpath[LOVR_PATH_MAX];
  path_join(fullpath, archive->path, path);

  struct stat st;
  if (stat(fullpath, &st)) {
    return 0;
  }

  return st.st_mtimespec.tv_sec;
}

static void* dirRead(Archive* archive, const char* path, size_t* bytesRead) {
  char fullpath[LOVR_PATH_MAX];
  path_join(fullpath, archive->path, path);
  FILE* file = fopen(fullpath, "r");
  if (!file) return NULL;

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  rewind(file);

  char* data = malloc(size);
  if (!data) {
    fclose(file);
    return NULL;
  }

  *bytesRead = fread(data, sizeof(char), size, file);
  fclose(file);
  return data;
}

static void dirUnmount(Archive* archive) {
  free(archive);
}

Archive* lovrFilesystemMountDir(const char* path) {
  struct stat st;
  if (stat(path, &st) || !S_ISDIR(st.st_mode)) {
    return NULL;
  }

  Archive* archive = malloc(sizeof(Archive));
  if (!archive) return NULL;

  archive->type = ARCHIVE_DIR;
  archive->path = path;
  archive->exists = dirExists;
  archive->getDirectoryItems = dirGetDirectoryItems;
  archive->getSize = dirGetSize;
  archive->isDirectory = dirIsDirectory;
  archive->isFile = dirIsFile;
  archive->lastModified = dirLastModified;
  archive->read = dirRead;
  archive->unmount = dirUnmount;

  return archive;
}
