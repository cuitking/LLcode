
#include "common.h"
#include "utils.h"

extern void bson_to_lua(lua_State* L, const mongo::BSONObj& obj);

namespace
{
	inline mongo::DBClientCursor* userdata_to_cursor(lua_State* L, int index)
	{
		void* ud = 0;
		ud = luaL_checkudata(L, index, LUAMONGO_CURSOR);
		mongo::DBClientCursor* cursor = *((mongo::DBClientCursor**)ud);
		return cursor;
	}
} // anonymous namespace

/*
 * res = cursor:next()
 */
static int cursor_next(lua_State* L)
{
	mongo::DBClientCursor* cursor = userdata_to_cursor(L, 1);

	if (cursor->more())
	{
		bson_to_lua(L, cursor->next());
	}
	else
	{
		lua_pushnil(L);
	}

	return 1;
}

static int result_iterator(lua_State* L)
{
	mongo::DBClientCursor* cursor = userdata_to_cursor(L, lua_upvalueindex(1));

	if (cursor->more())
	{
		bson_to_lua(L, cursor->next());
	}
	else
	{
		lua_pushnil(L);
	}

	return 1;
}

/*
 * iter_func = cursor:results()
 */
static int cursor_results(lua_State* L)
{
	lua_pushcclosure(L, result_iterator, 1);
	return 1;
}

/*
 * has_more = cursor:has_more(in_current_batch)
 *    pass true to call moreInCurrentBatch (mongo >=1.5)
 */
static int cursor_has_more(lua_State* L)
{
	mongo::DBClientCursor* cursor = userdata_to_cursor(L, 1);
	bool in_current_batch = lua_toboolean(L, 2) != 0;
	if (in_current_batch)
	{
		lua_pushboolean(L, cursor->moreInCurrentBatch());
	}
	else
	{
		lua_pushboolean(L, cursor->more());
	}
	return 1;
}

/*
 * it_count = cursor:itcount()
 */
static int cursor_itcount(lua_State* L)
{
	mongo::DBClientCursor* cursor = userdata_to_cursor(L, 1);
	lua_pushinteger(L, cursor->itcount());
	return 1;
}

/*
 * is_dead = cursor:is_dead()
 */
static int cursor_is_dead(lua_State* L)
{
	mongo::DBClientCursor* cursor = userdata_to_cursor(L, 1);
	lua_pushboolean(L, cursor->isDead() ? 1 : 0);
	return 1;
}

/*
 * is_tailable = cursor:is_tailable()
 */
static int cursor_is_tailable(lua_State* L)
{
	mongo::DBClientCursor* cursor = userdata_to_cursor(L, 1);
	lua_pushboolean(L, cursor->tailable() ? 1 : 0);
	return 1;
}

/*
 * has_result_flag = cursor:has_result_flag()
 */
static int cursor_has_result_flag(lua_State* L)
{
	mongo::DBClientCursor* cursor = userdata_to_cursor(L, 1);
	int flag = lua_tointeger(L, 2);
	lua_pushboolean(L, cursor->hasResultFlag(flag) ? 1 : 0);
	return 1;
}

/*
 * id = cursor:get_id()
 */
static int cursor_get_id(lua_State* L)
{
	mongo::DBClientCursor* cursor = userdata_to_cursor(L, 1);
	lua_pushinteger(L, cursor->getCursorId());
	return 1;
}
/*
 * __gc
 */
static int cursor_gc(lua_State* L)
{
	mongo::DBClientCursor* cursor = userdata_to_cursor(L, 1);
	delete cursor;
	return 0;
}

/*
 * __tostring
 */
static int cursor_tostring(lua_State* L)
{
	mongo::DBClientCursor* cursor = userdata_to_cursor(L, 1);
	lua_pushfstring(L, "%s: %p", LUAMONGO_CURSOR, cursor);
	return 1;
}
/*
*取到cursor里的所有值
*返回一个Lua表
*/
static int cursor_all_results(lua_State* L)
{
	mongo::DBClientCursor* cursor = userdata_to_cursor(L, 1);
	int count = 0;
	lua_newtable(L);
	while (cursor->more())
	{
		count = count + 1;
		lua_pushnumber(L, count);//key
		const mongo::BSONObj& obj = cursor->next();
		bson_to_lua(L, obj); //value
		lua_rawset(L, -3);
	}
	return 1;
}
int mongo_cursor_register(lua_State* L)
{
	static const luaL_Reg cursor_methods[] =
	{
		{"next", cursor_next},
		{"results", cursor_results},
		{"has_more", cursor_has_more},
		{"itcount", cursor_itcount},
		{"is_dead", cursor_is_dead},
		{"is_tailable", cursor_is_tailable},
		{"has_result_flag", cursor_has_result_flag},
		{"get_id", cursor_get_id},
		{"all_results", cursor_all_results},
		{NULL, NULL}
	};

	static const luaL_Reg cursor_class_methods[] =
	{
		{NULL, NULL}
	};

	luaL_newmetatable(L, LUAMONGO_CURSOR);
	luaL_register(L, 0, cursor_methods);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	lua_pushcfunction(L, cursor_gc);
	lua_setfield(L, -2, "__gc");

	lua_pushcfunction(L, cursor_tostring);
	lua_setfield(L, -2, "__tostring");

	luaL_register(L, LUAMONGO_CURSOR, cursor_class_methods);

	return 1;
}
