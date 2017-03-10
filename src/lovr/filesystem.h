#include "luax.h"

extern const luaL_Reg lovrFilesystem[];
int l_lovrFilesystemInit(lua_State* L);
int l_lovrFilesystemAppend(lua_State* L);
int l_lovrFilesystemCreateDirectory(lua_State* L);
int l_lovrFilesystemExists(lua_State* L);
int l_lovrFilesystemGetAppdataDirectory(lua_State* L);
int l_lovrFilesystemGetExecutablePath(lua_State* L);
int l_lovrFilesystemGetIdentity(lua_State* L);
int l_lovrFilesystemGetSaveDirectory(lua_State* L);
int l_lovrFilesystemIsDirectory(lua_State* L);
int l_lovrFilesystemIsFile(lua_State* L);
int l_lovrFilesystemLoad(lua_State* L);
int l_lovrFilesystemMount(lua_State* L);
int l_lovrFilesystemRead(lua_State* L);
int l_lovrFilesystemRemove(lua_State* L);
int l_lovrFilesystemSetIdentity(lua_State* L);
int l_lovrFilesystemUnmount(lua_State* L);
int l_lovrFilesystemWrite(lua_State* L);
