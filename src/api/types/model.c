#include "api/lovr.h"
#include "graphics/model.h"

int l_lovrModelDraw(lua_State* L) {
  Model* model = luax_checktype(L, 1, Model);
  float transform[16];
  luax_readtransform(L, 2, transform, 1);
  lovrModelDraw(model, transform);
  return 0;
}

int l_lovrModelGetMaterial(lua_State* L) {
  Model* model = luax_checktype(L, 1, Model);
  Material* material = lovrModelGetMaterial(model);
  luax_pushtype(L, Material, material);
  return 1;
}

int l_lovrModelSetMaterial(lua_State* L) {
  Model* model = luax_checktype(L, 1, Model);
  Material* material = luax_checktype(L, 2, Material);
  lovrModelSetMaterial(model, material);
  return 0;
}

int l_lovrModelGetAABB(lua_State* L) {
  Model* model = luax_checktype(L, 1, Model);
  float* aabb = lovrModelGetAABB(model);
  for (int i = 0; i < 6; i++) {
    lua_pushnumber(L, aabb[i]);
  }
  return 6;
}

const luaL_Reg lovrModel[] = {
  { "draw", l_lovrModelDraw },
  { "getMaterial", l_lovrModelGetMaterial },
  { "setMaterial", l_lovrModelSetMaterial },
  { "getAABB", l_lovrModelGetAABB },
  { NULL, NULL }
};
