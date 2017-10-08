#include "util.h"
#include "lib/map/map.h"
#include "graphics/shader.h"

#pragma once

typedef map_t(UniformValue) map_uniform_value_t;

typedef struct {
  Ref ref;
  Shader* shader;
  map_uniform_value_t values;
} Material;

Material* lovrMaterialCreate(Shader* shader);
void lovrMaterialDestroy(const Ref* ref);
Shader* lovrMaterialGetShader(Material* material);
void lovrMaterialBind(Material* material);
