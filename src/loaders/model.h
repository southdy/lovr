#include "filesystem/blob.h"
#include "util.h"
#include "lib/vec/vec.h"

#pragma once

typedef struct {
  // material
  int drawStart;
  int drawCount;
} ModelPrimitive;

typedef struct ModelNode {
  float transform[16];
  struct ModelNode* parent;
  struct ModelNode** children;
  int childCount;
  ModelPrimitive** primitives;
  int primitiveCount;
} ModelNode;

typedef struct {
  ModelNode* root;
  float* vertexData;
  int vertexSize;
  uint32_t vertexCount;
  uint32_t* indexData;
  uint32_t indexCount;
  int hasNormals;
  int hasUVs;
} ModelData;

ModelData* lovrModelDataCreate(Blob* blob);
void lovrModelDataDestroy(ModelData* modelData);
