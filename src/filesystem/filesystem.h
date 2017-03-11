#include "vendor/vec/vec.h"
#include "vendor/map/map.h"
#include "vendor/microtar/microtar.h"
#include <stdio.h>

#pragma once

#define LOVR_PATH_MAX 1024

typedef void getDirectoryItemsCallback(const char* path, void* userdata);

typedef enum {
  ARCHIVE_FS,
  ARCHIVE_TAR
} ArchiveType;

typedef struct Archive {
  ArchiveType type;
  const char* path;
  void* userdata;
  int (*exists)(struct Archive* archive, const char* path);
  void (*getDirectoryItems)(struct Archive* archive, const char* path, getDirectoryItemsCallback callback, void* userdata);
  int (*getSize)(struct Archive* archive, const char* path, size_t* size);
  int (*isDirectory)(struct Archive* archive, const char* path);
  int (*isFile)(struct Archive* archive, const char* path);
  int (*lastModified)(struct Archive* archive, const char* path);
  void* (*read)(struct Archive* archive, const char* path, size_t* bytesRead);
  void (*unmount)(struct Archive* archive);
} Archive;

typedef vec_t(Archive*) vec_archive_t;

typedef struct {
  mtar_t mtar;
  map_int_t entries;
  size_t offset;
  FILE* file;
} TarArchive;

typedef struct {
  vec_archive_t archives;
  char* writePath;
  const char* identity;
  char* source;
  int isFused;
} FilesystemState;

void lovrFilesystemInit(const char* argv1);
void lovrFilesystemDestroy();
int lovrFilesystemCreateDirectory(const char* path);
int lovrFilesystemExists(const char* path);
int lovrFilesystemGetAppdataDirectory(char* dest, unsigned int size);
void lovrFilesystemGetDirectoryItems(const char* path, getDirectoryItemsCallback callback, void* userdata);
int lovrFilesystemGetExecutablePath(char* dest, unsigned int size);
const char* lovrFilesystemGetIdentity();
int lovrFilesystemGetLastModified(const char* path);
const char* lovrFilesystemGetRealDirectory(const char* path);
const char* lovrFilesystemGetSaveDirectory();
int lovrFilesystemGetSize(const char* path, size_t* size);
const char* lovrFilesystemGetSource();
int lovrFilesystemIsDirectory(const char* path);
int lovrFilesystemIsFile(const char* path);
int lovrFilesystemIsFused();
int lovrFilesystemMount(const char* source, int append);
void* lovrFilesystemRead(const char* path, size_t* bytesRead);
int lovrFilesystemRemove(const char* path);
void lovrFilesystemSetIdentity(const char* identity);
int lovrFilesystemUnmount(const char* path);
size_t lovrFilesystemWrite(const char* path, const char* content, size_t size, int append);
