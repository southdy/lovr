#include "filesystem/filesystem.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#include <initguid.h>
#include <KnownFolders.h>
#include <ShlObj.h>
#include <wchar.h>
#else
#include <pwd.h>
#include <unistd.h>
#include <errno.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#define FOREACH_ARCHIVE(archives) Archive* archive; int i; vec_foreach(archives, archive, i)

static FilesystemState state;

void lovrFilesystemInit(const char* argv1) {
  vec_init(&state.archives);
  state.writePath = NULL;
  state.identity = NULL;
  state.source = NULL;
  state.isFused = 1;

  // Try to load an archive fused to the executable
  char executable[LOVR_PATH_MAX];
  lovrFilesystemGetExecutablePath(executable, LOVR_PATH_MAX);
  if (lovrFilesystemMount(executable, 1)) {

    // If that didn't work, try to load a folder/archive specified on the command line
    state.isFused = 0;
    state.source = realpath(argv1, NULL);
    path_normalize(state.source);
    if (!state.source || lovrFilesystemMount(state.source, 1)) {
      free(state.source);
      state.source = NULL;
    }
  } else {
    state.source = malloc(LOVR_PATH_MAX * sizeof(char));
    strncpy(state.source, executable, LOVR_PATH_MAX);
  }

  atexit(lovrFilesystemDestroy);
}

void lovrFilesystemDestroy() {
  FOREACH_ARCHIVE(&state.archives) {
    archive->unmount(archive);
  }
  vec_deinit(&state.archives);
  free(state.writePath);
  free(state.source);
}

int lovrFilesystemCreateDirectory(const char* path) {
  if (!state.writePath) {
    return 1;
  }

  char fullpath[LOVR_PATH_MAX];
  path_join(fullpath, state.writePath, path);
  return mkdir_p(fullpath);
}

int lovrFilesystemExists(const char* path) {
  FOREACH_ARCHIVE(&state.archives) {
    if (archive->exists(archive, path)) {
      return 1;
    }
  }

  return 0;
}

int lovrFilesystemGetAppdataDirectory(char* dest, unsigned int size) {
#ifdef __APPLE__
  const char* home;
  if ((home = getenv("HOME")) == NULL) {
    home = getpwuid(getuid())->pw_dir;
  }

  snprintf(dest, size, "%s/Library/Application Support", home);
  path_normalize(dest);
  return 0;
#elif _WIN32
  PWSTR appData = NULL;
  SHGetKnownFolderPath(&FOLDERID_RoamingAppData, 0, NULL, &appData);
  wcstombs(dest, appData, size);
  path_normalize(dest);
  CoTaskMemFree(appData);
  return 0;
#else
#error "This platform is missing an implementation for lovrFilesystemGetAppdataDirectory"
#endif

  return 1;
}

void lovrFilesystemGetDirectoryItems(const char* path, getDirectoryItemsCallback callback, void* userdata) {
  FOREACH_ARCHIVE(&state.archives) {
    archive->getDirectoryItems(archive, path, callback, userdata);
  }
}

int lovrFilesystemGetExecutablePath(char* dest, unsigned int size) {
#ifdef __APPLE__
  if (_NSGetExecutablePath(dest, &size) == 0) {
    path_normalize(dest);
    return 0;
  }
#elif _WIN32
  int err = GetModuleFileName(NULL, dest, size);
  if (!err) {
    path_normalize(dest);
    return 0;
  }
#else
#error "This platform is missing an implementation for lovrFilesystemGetExecutablePath"
#endif

  return 1;
}

const char* lovrFilesystemGetIdentity() {
  return state.identity;
}

int lovrFilesystemGetLastModified(const char* path) {
  FOREACH_ARCHIVE(&state.archives) {
    int lastModified = archive->lastModified(archive, path);
    if (lastModified) {
      return lastModified;
    }
  }

  return 0;
}

const char* lovrFilesystemGetRealDirectory(const char* path) {
  FOREACH_ARCHIVE(&state.archives) {
    if (archive->exists(archive, path)) {
      return archive->path;
    }
  }

  return NULL;
}

const char* lovrFilesystemGetSaveDirectory() {
  return state.writePath;
}

int lovrFilesystemGetSize(const char* path, size_t* size) {
  FOREACH_ARCHIVE(&state.archives) {
    if (!archive->getSize(archive, path, size)) {
      return 0;
    }
  }

  *size = 0;
  return 1;
}

const char* lovrFilesystemGetSource() {
  return state.source;
}

int lovrFilesystemIsDirectory(const char* path) {
  FOREACH_ARCHIVE(&state.archives) {
    if (archive->isDirectory(archive, path)) {
      return 1;
    }
  }

  return 0;
}

int lovrFilesystemIsFile(const char* path) {
  FOREACH_ARCHIVE(&state.archives) {
    if (archive->isFile(archive, path)) {
      return 1;
    }
  }

  return 0;
}

int lovrFilesystemIsFused(const char* path) {
  return state.isFused;
}

int lovrFilesystemMount(const char* path, int append) {
  FOREACH_ARCHIVE(&state.archives) {
    if (!strncmp(archive->path, path, LOVR_PATH_MAX)) {
      return 1;
    }
  }

  archive = NULL; // FOREACH_ARCHIVE defines this...
  if ((archive = lovrFilesystemMountDir(path)) != NULL || (archive = lovrFilesystemMountTar(path)) != NULL) {
    if (append) {
      vec_push(&state.archives, archive);
    } else {
      vec_insert(&state.archives, 0, archive);
    }
  }

  return !archive;
}

void* lovrFilesystemRead(const char* path, size_t* bytesRead) {
  void* data;

  FOREACH_ARCHIVE(&state.archives) {
    if ((data = archive->read(archive, path, bytesRead)) != NULL) {
      return data;
    }
  }

  *bytesRead = 0;
  return NULL;
}

int lovrFilesystemRemove(const char* path) {
  if (!state.writePath) {
    return 1;
  }

  char fullpath[LOVR_PATH_MAX];
  path_join(fullpath, state.writePath, path);
  return remove(fullpath);
}

void lovrFilesystemSetIdentity(const char* identity) {
  state.identity = identity;

  if (state.writePath) {
    lovrFilesystemUnmount(state.writePath);
  } else {
    state.writePath = malloc(LOVR_PATH_MAX * sizeof(char));
    if (!state.writePath) {
      error("Unable to allocate memory for save directory");
    }
  }

  int err = 0;
  err |= lovrFilesystemGetAppdataDirectory(state.writePath, LOVR_PATH_MAX);
  err |= snprintf(state.writePath, LOVR_PATH_MAX, "%s/LOVR/%s", state.writePath, identity) >= LOVR_PATH_MAX;
  err |= mkdir_p(state.writePath);
  err |= lovrFilesystemMount(state.writePath, 0);

  if (err) {
    error("Unable to create save directory");
  }
}

int lovrFilesystemUnmount(const char* path) {
  FOREACH_ARCHIVE(&state.archives) {
    if (!strncmp(archive->path, path, LOVR_PATH_MAX)) {
      archive->unmount(archive);
      vec_splice(&state.archives, i, 1);
      return 1;
    }
  }

  return 0;
}

size_t lovrFilesystemWrite(const char* path, const char* content, size_t size, int append) {
  if (!state.writePath) {
    return 0;
  }

  char fullpath[LOVR_PATH_MAX];
  path_join(fullpath, state.writePath, path);
  FILE* file = fopen(fullpath, append ? "a" : "w");
  if (!file) {
    return 0;
  }

  size_t bytesWritten = fwrite(content, sizeof(char), size, file);
  fclose(file);
  return bytesWritten;
}
