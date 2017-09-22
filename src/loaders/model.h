#include "filesystem/blob.h"
#include "util.h"
#include "lib/vec/vec.h"

#pragma once

typedef enum {
  PROPERTY_FLOAT,
  PROPERTY_COLOR,
  PROPERTY_TEXTURE
} MaterialPropertyType;

typedef struct {
  char name[100];
  MaterialPropertyType type;
  union {
    void* data;
    float* floats;
    int* ints;
  } value;
} ModelMaterial;

typedef vec_t(MaterialProperty) vec_material_property_t;

typedef struct {
  vec_material_property_t properties;
} ModelMaterial;

typedef struct {
  int material;
  int drawStart;
  int drawCount;
} ModelPrimitive;

typedef struct ModelNode {
  float transform[16];
  int parent;
  vec_uint_t children;
  vec_uint_t primitives;
} ModelNode;

typedef struct {
  ModelNode* nodes;
  ModelPrimitive* primitives;
  ModelMaterial* materials;
  float* vertices;
  uint32_t* indices;
  int nodeCount;
  int primitiveCount;
  int materialCount;
  int vertexSize;
  int vertexCount;
  int indexCount;
  int hasNormals;
  int hasUVs;
} ModelData;

ModelData* lovrModelDataCreate(Blob* blob);
void lovrModelDataDestroy(ModelData* modelData);
