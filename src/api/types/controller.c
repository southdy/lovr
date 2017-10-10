#include "api/lovr.h"
#include "headset/headset.h"
#include "loaders/model.h"
#include "loaders/texture.h"
#include "graphics/model.h"

int l_lovrControllerIsPresent(lua_State* L) {
  Controller* controller = luax_checktype(L, 1, Controller);
  lua_pushboolean(L, lovrHeadsetControllerIsPresent(controller));
  return 1;
}

int l_lovrControllerGetHand(lua_State* L) {
  Controller* controller = luax_checktype(L, 1, Controller);
  ControllerHand hand = lovrHeadsetControllerGetHand(controller);
  luax_pushenum(L, &ControllerHands, hand);
  return 1;
}

int l_lovrControllerGetPosition(lua_State* L) {
  Controller* controller = luax_checktype(L, 1, Controller);
  float x, y, z;
  lovrHeadsetControllerGetPosition(controller, &x, &y, &z);
  lua_pushnumber(L, x);
  lua_pushnumber(L, y);
  lua_pushnumber(L, z);
  return 3;
}

int l_lovrControllerGetOrientation(lua_State* L) {
  Controller* controller = luax_checktype(L, 1, Controller);
  float angle, x, y, z;
  lovrHeadsetControllerGetOrientation(controller, &angle, &x, &y, &z);
  lua_pushnumber(L, angle);
  lua_pushnumber(L, x);
  lua_pushnumber(L, y);
  lua_pushnumber(L, z);
  return 4;
}

int l_lovrControllerGetAxis(lua_State* L) {
  Controller* controller = luax_checktype(L, 1, Controller);
  ControllerAxis* axis = (ControllerAxis*) luax_checkenum(L, 2, &ControllerAxes, "controller axis");
  lua_pushnumber(L, lovrHeadsetControllerGetAxis(controller, *axis));
  return 1;
}

int l_lovrControllerIsDown(lua_State* L) {
  Controller* controller = luax_checktype(L, 1, Controller);
  ControllerButton* button = (ControllerButton*) luax_checkenum(L, 2, &ControllerButtons, "controller button");
  lua_pushboolean(L, lovrHeadsetControllerIsDown(controller, *button));
  return 1;
}

int l_lovrControllerIsTouched(lua_State* L) {
  Controller* controller = luax_checktype(L, 1, Controller);
  ControllerButton* button = (ControllerButton*) luax_checkenum(L, 2, &ControllerButtons, "controller button");
  lua_pushboolean(L, lovrHeadsetControllerIsTouched(controller, *button));
  return 1;
}

int l_lovrControllerVibrate(lua_State* L) {
  Controller* controller = luax_checktype(L, 1, Controller);
  float duration = luaL_optnumber(L, 2, .5);
  float power = luaL_optnumber(L, 3, 1);
  lovrHeadsetControllerVibrate(controller, duration, power);
  return 0;
}

int l_lovrControllerNewModel(lua_State* L) {
  Controller* controller = luax_checktype(L, 1, Controller);
  ModelData* modelData = lovrHeadsetControllerNewModelData(controller);
  //TextureData* textureData = lovrHeadsetControllerNewTextureData(controller);
  if (modelData) {
    Model* model = lovrModelCreate(modelData);
    /*
    if (textureData) {
      Texture* texture = lovrTextureCreate(TEXTURE_2D, &textureData, 1);
      lovrModelSetTexture(model, texture);
      lovrRelease(&texture->ref);
    }
    */
    luax_pushtype(L, Model, model);
    lovrRelease(&model->ref);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

const luaL_Reg lovrController[] = {
  { "isPresent", l_lovrControllerIsPresent },
  { "getHand", l_lovrControllerGetHand },
  { "getPosition", l_lovrControllerGetPosition },
  { "getOrientation", l_lovrControllerGetOrientation },
  { "getAxis", l_lovrControllerGetAxis },
  { "isDown", l_lovrControllerIsDown },
  { "isTouched", l_lovrControllerIsTouched },
  { "vibrate", l_lovrControllerVibrate },
  { "newModel", l_lovrControllerNewModel },
  { NULL, NULL }
};
