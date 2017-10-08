#include "api/lovr.h"
#include "math/transform.h"

int l_lovrShaderSend(lua_State* L) {
  Shader* shader = luax_checktype(L, 1, Shader);
  const char* name = luaL_checkstring(L, 2);
  int n;
  lua_settop(L, 3);

  Uniform* uniform = lovrShaderGetUniform(shader, name);

  if (!uniform) {
    return luaL_error(L, "Unknown shader variable '%s'", name);
  }

  UniformValue value = uniform->value;
  int components = uniform->components;

  if (components > 1) {
    luaL_checktype(L, 3, LUA_TTABLE);
    lua_rawgeti(L, 3, 1);
    if (!lua_istable(L, -1)) {
      lua_newtable(L);
      lua_pushvalue(L, 3);
      lua_rawseti(L, -2, 1);
    } else {
      lua_pop(L, 1);
    }

    n = lua_objlen(L, -1);
    if (n != uniform->count) {
      const char* elements = uniform->count == 1 ? "element" : "elements";
      return luaL_error(L, "Expected %d %s for array '%s', got %d", uniform->count, elements, uniform->name, n);
    }
  }

  switch (uniform->type) {
    case UNIFORM_INT:
    case UNIFORM_SAMPLER:
      if (components == 1) {
        value.ints[0] = luaL_checkinteger(L, 3);
      } else {
        luaL_checktype(L, 3, LUA_TTABLE);
        for (int i = 0; i < n; i++) {
          lua_rawgeti(L, -1, i + 1);
          if (lua_type(L, -1) != LUA_TTABLE || (int) lua_objlen(L, -1) != components) {
            return luaL_error(L, "Expected %d components for uniform '%s' #%d, got %d", components, uniform->name, lua_objlen(L, -1));
          }
          for (int j = 0; j < components; j++) {
            lua_rawgeti(L, -1, j + 1);
            value.ints[i * components + j] = luaL_checkinteger(L, -1);
            lua_pop(L, 1);
          }
          lua_pop(L, 1);
        }
      }
      break;

    case UNIFORM_FLOAT:
      if (components == 1) {
        value.floats[0] = luaL_checknumber(L, 3);
      } else {
        luaL_checktype(L, 3, LUA_TTABLE);
        for (int i = 0; i < n; i++) {
          lua_rawgeti(L, -1, i + 1);
          if (lua_type(L, -1) != LUA_TTABLE || (int) lua_objlen(L, -1) != components) {
            return luaL_error(L, "Expected %d components for uniform '%s', got %d", components, uniform->name, lua_objlen(L, -1));
          }
          for (int j = 0; j < components; j++) {
            lua_rawgeti(L, -1, j + 1);
            value.floats[i * components + j] = luaL_checknumber(L, -1);
            lua_pop(L, 1);
          }
          lua_pop(L, 1);
        }
      }
      break;

    case UNIFORM_MATRIX:
      if (components == 4 && lua_isuserdata(L, 3)) {
        Transform* transform = luax_checktype(L, 3, Transform);
        memcpy(value.floats, transform->matrix, 16 * sizeof(float));
      } else {
        luaL_checktype(L, 3, LUA_TTABLE);
        for (int i = 0; i < n; i++) {
          lua_rawgeti(L, -1, i + 1);
          for (int j = 0; j < components * components; j++) {
            lua_rawgeti(L, -1, j + 1);
            value.floats[i * components * components + j] = luaL_checknumber(L, -1);
            lua_pop(L, 1);
          }
          lua_pop(L, 1);
        }
      }
      break;
  }

  lovrShaderSetUniformValue(shader, name, value);
  return 0;
}

const luaL_Reg lovrShader[] = {
  { "send", l_lovrShaderSend },
  { NULL, NULL }
};
