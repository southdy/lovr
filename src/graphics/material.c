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
  /*
  const char* key;
  map_iter_t iter = map_iter(&material->values);
  while ((key = map_next(&material->values, &iter))) {
    UniformValue* value = map_get(&material->values, key);
    lovrShaderSetUniformValue(material->shader, key, *value);
  }
  */
}

void lovrMaterialSetFloats(Material* material, const char* name, float* data, int count) {
  UniformValue* value = map_get(&material->values, name);

  if (value) {
    memcpy(value->floats, data, count * sizeof(float));
  }
}

void lovrMaterialSetInts(Material* material, const char* name, int* data, int count) {
  UniformValue* value = map_get(&material->values, name);

  if (value) {
    memcpy(value->ints, data, count * sizeof(int));
  }
}

void lovrMaterialSetTextures(Material* material, const char* name, Texture** data, int count) {
  UniformValue* value = map_get(&material->values, name);

  if (value) {
    memcpy(value->textures, data, count * sizeof(Texture*));
  }
}
