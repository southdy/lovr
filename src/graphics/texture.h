#include "loaders/texture.h"
#include "lib/glfw.h"
#include "util.h"

#pragma once

typedef enum {
  FILTER_NEAREST,
  FILTER_BILINEAR,
  FILTER_TRILINEAR,
  FILTER_ANISOTROPIC
} FilterMode;

typedef struct {
  FilterMode mode;
  float anisotropy;
} TextureFilter;

typedef enum {
  WRAP_CLAMP = GL_CLAMP_TO_EDGE,
  WRAP_REPEAT = GL_REPEAT,
  WRAP_MIRRORED_REPEAT = GL_MIRRORED_REPEAT
} WrapMode;

typedef enum {
  PROJECTION_ORTHOGRAPHIC,
  PROJECTION_PERSPECTIVE
} TextureProjection;

typedef struct {
  Ref ref;
  TextureData* textureData;
  GLuint id;
  GLuint msaaId;
  GLuint framebuffer;
  GLuint resolveFramebuffer;
  GLuint depthBuffer;
  TextureProjection projection;
  TextureFilter filter;
  WrapMode wrapHorizontal;
  WrapMode wrapVertical;
  int msaa;
} Texture;

GLenum lovrTextureGetGLFormat(TextureFormat format);

Texture* lovrTextureCreate(TextureData* textureData);
Texture* lovrTextureCreateWithFramebuffer(TextureData* textureData, TextureProjection projection, int msaa);
void lovrTextureDestroy(const Ref* ref);
void lovrTextureBindFramebuffer(Texture* texture);
void lovrTextureResolveMSAA(Texture* texture);
void lovrTextureRefresh(Texture* texture);
int lovrTextureGetHeight(Texture* texture);
int lovrTextureGetWidth(Texture* texture);
TextureFilter lovrTextureGetFilter(Texture* texture);
void lovrTextureSetFilter(Texture* texture, TextureFilter filter);
void lovrTextureGetWrap(Texture* texture, WrapMode* horizontal, WrapMode* vertical);
void lovrTextureSetWrap(Texture* texture, WrapMode horizontal, WrapMode vertical);
