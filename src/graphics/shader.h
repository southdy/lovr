#include "graphics/texture.h"
#include "math/math.h"
#include "lib/map/map.h"
#include "lib/glfw.h"
#include "util.h"

#pragma once

#define LOVR_SHADER_POSITION 0
#define LOVR_SHADER_NORMAL 1
#define LOVR_SHADER_TEX_COORD 2
#define LOVR_MAX_UNIFORM_LENGTH 256

typedef enum {
  SHADER_DEFAULT,
  SHADER_SKYBOX,
  SHADER_FONT,
  SHADER_FULLSCREEN
} DefaultShader;

typedef enum {
  UNIFORM_FLOAT,
  UNIFORM_MATRIX,
  UNIFORM_INT,
  UNIFORM_SAMPLER
} UniformType;

typedef union {
  void* data;
  float* floats;
  int* ints;
  Texture** textures;
} UniformValue;

typedef struct {
  GLchar name[LOVR_MAX_UNIFORM_LENGTH];
  int index;
  int location;
  UniformType type;
  int components;
  int count;
  size_t size;
  UniformValue value;
  Texture** textures;
  int textureUnitOffset;
  int dirty;
} Uniform;

typedef map_t(Uniform) map_uniform_t;

typedef struct {
  Ref ref;
  uint32_t program;
  map_uniform_t uniforms;
} Shader;

Shader* lovrShaderCreate(const char* vertexSource, const char* fragmentSource);
Shader* lovrShaderCreateDefault(DefaultShader type);
void lovrShaderDestroy(const Ref* ref);
void lovrShaderBind(Shader* shader, int force);
int lovrShaderGetAttributeId(Shader* shader, const char* name);
Uniform* lovrShaderGetUniform(Shader* shader, const char* name);
void lovrShaderUpdateUniform(Shader* shader, const char* name, UniformValue value);
