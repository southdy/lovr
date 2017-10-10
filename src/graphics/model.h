#include "loaders/model.h"
#include "graphics/mesh.h"
#include "graphics/texture.h"
#include "math/math.h"
#include "lib/glfw.h"
#include "util.h"

#pragma once

typedef struct {
  Ref ref;
  ModelData* modelData;
  Mesh* mesh;
  Material* material;
  float aabb[6];
} Model;

Model* lovrModelCreate(ModelData* modelData);
void lovrModelDestroy(const Ref* ref);
void lovrModelDraw(Model* model, mat4 transform);
Material* lovrModelGetMaterial(Model* model);
void lovrModelSetMaterial(Model* model, Material* material);
float* lovrModelGetAABB(Model* model);
