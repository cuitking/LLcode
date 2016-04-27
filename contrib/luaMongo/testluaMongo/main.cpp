extern "C" {
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>

#if !defined(LUA_VERSION_NUM) || (LUA_VERSION_NUM < 501)
#include <compat-5.1.h>
#endif
};

#include <iostream>
#include <Windows.h>

int main()
{
	lua_State* L = lua_open();
	luaL_openlibs(L);
	if (luaL_loadfile(L, "test.lua") || lua_pcall(L, 0, 0, 0))
	{
		std::cout << lua_tostring(L, -1) << std::endl;
	}

	while (1)
	{
		lua_getglobal(L, "Update");
		lua_pcall(L, 0, 0, 0);
		Sleep(100);
	}
	lua_close(L);
	return 0;
}