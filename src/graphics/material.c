#include "graphics/material.h"

Material* lovrMaterialCreate(Shader* shader) {
  Material* material = lovrAlloc(sizeof(Material), lovrMaterialDestroy);
  if (!material) return NULL;

  lovrRetain(&shader->ref);
  material->shader = shader;
  map_init(&material->values);

  return material;
}

void lovrMaterialDestroy(const Ref* ref) {
  Material* material = containerof(ref, Material);
  lovrRelease(&material->shader->ref);
  map_deinit(&material->values);
  free(material);
}

Shader* lovrMaterialGetShader(Material* material) {
  return material->shader;
}

void lovrMaterialBind(Material* material) {
  const char* key;
  map_iter_t iter = map_iter(&material->values);
  while ((key = map_next(&material->values, &iter))) {
    UniformValue* value = map_get(&material->values, key);
    lovrShaderSetUniformValue(material->shader, key, *value);
  }
}

UniformValue* lovrMaterialGetValue(Material* material, const char* uniform) {
  return map_get(&material->values, uniform);
}
