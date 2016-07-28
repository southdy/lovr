#include "timer.h"
#include "lovr.h"
#include "glfw.h"

int lovrTimerStep(lua_State* L) {
  lua_pushnumber(L, glfwGetTime());
  glfwSetTime(0);

  return 1;
}

const luaL_Reg lovrTimer[] = {
  { "step", lovrTimerStep },
  { NULL, NULL }
};