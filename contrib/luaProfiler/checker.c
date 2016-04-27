#include "checker.h"
#include <lua.h>
#include <src/lj_obj.h>

void checker_hook(lua_State* L, lua_Debug* ar);

int checker_start(lua_State* L)
{
	int hookmask = lua_gethookmask(L) | LUA_MASKRET;
	lua_pushboolean(L, lua_sethook(L, checker_hook, hookmask, 0));
	return 1;
}

int checker_stop(lua_State* L)
{
	int hookmask = lua_gethookmask(L) & ~LUA_MASKRET;
	lua_pushboolean(L, lua_sethook(L, checker_hook, hookmask, 0));
	return 1;
}

void checker_hook(lua_State* L, lua_Debug* ar)
{
	if (ar->event == LUA_HOOKRET)
	{
		global_State* g = G(L);
		GCobj* o = gcref(g->gc.root);

		for (; o != NULL;)
		{
			if (!((~(o->gch.gct) - LJ_TISGCV) > (LJ_TNUMX - LJ_TISGCV)))
			{
				return;
			}
			o = gcnext(o);
		}
	}
}

