#include "filesystem/filesystem.h"
#include "util.h"
#include "vendor/microtar/microtar.h"
#include "vendor/map/map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

typedef struct {
  Archive archive;
  mtar_t mtar;
  map_int_t entries;
  size_t offset;
  FILE* file;
} TarArchive;

static int tarLoad(Archive* archive, const char* path, mtar_header_t* header) {
  TarArchive* tar = (TarArchive*) archive;
  int* pos = map_get(&tar->entries, path);
  if (!pos) {
    return 1;
  }

  mtar_seek(&tar->mtar, (unsigned) *pos);
  return mtar_read_header(&tar->mtar, header);
}

static int tarExists(Archive* archive, const char* path) {
  TarArchive* tar = (TarArchive*) archive;
  return map_get(&tar->entries, path) != NULL;
}

static void tarGetDirectoryItems(Archive* archive, const char* path, getDirectoryItemsCallback callback, void* userdata) {
  TarArchive* tar = (TarArchive*) archive;
  map_iter_t iter = map_iter(&tar->entries);
  const char* key;
  char tmp[LOVR_PATH_MAX];
  while ((key = map_next(&tar->entries, &iter)) != NULL) {
    if (strcmp(key, ".") && strcmp(key, "..")) {
      strncpy(tmp, key, LOVR_PATH_MAX);
      if (!strcmp(dirname(tmp), path)) {
        callback(path, userdata);
      }
    }
  }
}

static int tarGetSize(Archive* archive, const char* path, size_t* size) {
  mtar_header_t header;
  if (tarLoad(archive, path, &header)) {
    return 1;
  }

  *size = header.size;
  return 0;
}

static int tarIsDirectory(Archive* archive, const char* path) {
  mtar_header_t header;
  return !tarLoad(archive, path, &header) && header.type == MTAR_TDIR;
}

static int tarIsFile(Archive* archive, const char* path) {
  mtar_header_t header;
  return !tarLoad(archive, path, &header) && header.type == MTAR_TREG;
}

static int tarLastModified(Archive* archive, const char* path) {
  mtar_header_t header;
  if (tarLoad(archive, path, &header)) {
    return 0;
  }

  return header.mtime;
}

static void* tarRead(Archive* archive, const char* path, size_t* bytesRead) {
  TarArchive* tar = (TarArchive*) archive;
  mtar_header_t header;
  if (tarLoad(archive, path, &header)) {
    *bytesRead = 0;
    return NULL;
  }

  char* data = calloc(1, header.size + 1);
  if (mtar_read_data(&tar->mtar, data, header.size)) {
    *bytesRead = 0;
    free(data);
    return NULL;
  }

  *bytesRead = header.size;
  return data;
}

static void tarUnmount(Archive* archive) {
  TarArchive* tar = (TarArchive*) archive;
  fclose(tar->file);
  mtar_close(&tar->mtar);
  map_deinit(&tar->entries);
  free(tar);
  free(archive);
}

static int tarStreamRead(mtar_t* mtar, void* data, unsigned size) {
  TarArchive* tar = mtar->stream;
  unsigned res = fread(data, 1, size, tar->file);
  return (res == size) ? MTAR_ESUCCESS : MTAR_EREADFAIL;
}

static int tarStreamSeek(mtar_t* mtar, unsigned pos) {
  TarArchive* tar = mtar->stream;
  int err = fseek(tar->file, tar->offset + pos, SEEK_SET);
  return err ? MTAR_EFAILURE : MTAR_ESUCCESS;
}

static int tarStreamClose(mtar_t* mtar) {
  TarArchive* tar = mtar->stream;
  fclose(tar->file);
  return MTAR_ESUCCESS;
}

Archive* lovrFilesystemMountTar(const char* path) {
  TarArchive* tar = malloc(sizeof(TarArchive));
  Archive* archive = &tar->archive;

  if (!tar) return NULL;

  tar->offset = 0;
  tar->file = fopen(path, "rb");
  if (!tar->file) {
    return NULL;
  }

  // Initialize microtar
  memset(&tar->mtar, 0, sizeof(mtar_t));
  tar->mtar.stream = tar;
  tar->mtar.read = tarStreamRead;
  tar->mtar.seek = tarStreamSeek;
  tar->mtar.close = tarStreamClose;

  // If the beginning of the file does not have a header, check end of file for offset
  mtar_header_t header;
  if (mtar_read_header(&tar->mtar, &header)) {
    int offset;
    char buf[4];
    fseek(tar->file, -8, SEEK_END);
    fread(buf, 1, 4, tar->file);
    fread(&offset, 1, 4, tar->file);
    if (!memcmp(buf, "TAR\0", 4)) {
      fseek(tar->file, -offset, SEEK_END);
      tar->offset = ftell(tar->file);
    }
    mtar_rewind(&tar->mtar);

    // If there still isn't a valid header then this isn't a valid archive
    if (mtar_read_header(&tar->mtar, &header)) {
      fclose(tar->file);
      free(tar);
      return NULL;
    }
  }

  // Read all entries in the archive
  map_init(&tar->entries);
  while ((mtar_read_header(&tar->mtar, &header)) != MTAR_ENULLRECORD) {
    map_set(&tar->entries, header.name, tar->mtar.pos);
    mtar_next(&tar->mtar);
  }

  archive->type = ARCHIVE_TAR;
  archive->path = path;
  archive->exists = tarExists;
  archive->getDirectoryItems = tarGetDirectoryItems;
  archive->getSize = tarGetSize;
  archive->isDirectory = tarIsDirectory;
  archive->isFile = tarIsFile;
  archive->lastModified = tarLastModified;
  archive->read = tarRead;
  archive->unmount = tarUnmount;

  return archive;
}
