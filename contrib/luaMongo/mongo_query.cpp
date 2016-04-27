
#include "common.h"
#include "utils.h"

extern void lua_to_bson(lua_State* L, int stackpos, mongo::BSONObj& obj);
extern void bson_to_lua(lua_State* L, const mongo::BSONObj& obj);

namespace
{
	inline mongo::Query* userdata_to_query(lua_State* L, int index)
	{
		void* ud = 0;
		ud = luaL_checkudata(L, index, LUAMONGO_QUERY);
		mongo::Query* query = *((mongo::Query**)ud);
		return query;
	}
} // anonymous namespace

/*
 * query,err = mongo.Query.New(lua_table or json_str)
 */
static int query_new(lua_State* L)
{
	int resultcount = 1;
	int n = lua_gettop(L);

	try
	{
		mongo::Query** query = (mongo::Query**)lua_newuserdata(L, sizeof(mongo::Query*));

		if (n >= 1)
		{
			int type = lua_type(L, 1);
			if (type == LUA_TSTRING)
			{
				const char* jsonstr = luaL_checkstring(L, 1);
				*query = new mongo::Query(mongo::fromjson(jsonstr));
			}
			else if (type == LUA_TTABLE)
			{
				mongo::BSONObj data;
				lua_to_bson(L, 1, data);
				*query = new mongo::Query(data);
			}
			else
			{
				throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
			}
		}
		else
		{
			*query = new mongo::Query();
		}

		luaL_getmetatable(L, LUAMONGO_QUERY);
		lua_setmetatable(L, -2);
	}
	catch (std::exception& e)
	{
		lua_pushnil(L);
		lua_pushfstring(L, LUAMONGO_ERR_QUERY_FAILED, e.what());
		resultcount = 2;
	}
	catch (const char* err)
	{
		lua_pushnil(L);
		lua_pushstring(L, err);
		resultcount = 2;
	}

	return resultcount;
}

/*
 * ok,err = query:explain()
 */
static int query_explain(lua_State* L)
{
	mongo::Query* query = userdata_to_query(L, 1);

	try
	{
		query->explain();
	}
	catch (std::exception& e)
	{
		lua_pushboolean(L, 0);
		lua_pushfstring(L, LUAMONGO_ERR_QUERY_FAILED, e.what());
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

/*
 * ok,err = query:hint(lua_table or json_str)
 */
static int query_hint(lua_State* L)
{
	mongo::Query* query = userdata_to_query(L, 1);

	try
	{
		int type = lua_type(L, 2);
		if (type == LUA_TSTRING)
		{
			const char* jsonstr = luaL_checkstring(L, 2);
			query->hint(mongo::fromjson(jsonstr));
		}
		else if (type == LUA_TTABLE)
		{
			mongo::BSONObj data;
			lua_to_bson(L, 2, data);
			query->hint(data);
		}
		else
		{
			throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
		}
	}
	catch (std::exception& e)
	{
		lua_pushboolean(L, 0);
		lua_pushfstring(L, LUAMONGO_ERR_QUERY_FAILED, e.what());
		return 2;
	}
	catch (const char* err)
	{
		lua_pushboolean(L, 0);
		lua_pushstring(L, err);
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

/*
 * is_explain = query:is_explain()
 */
static int query_is_explain(lua_State* L)
{
	mongo::Query* query = userdata_to_query(L, 1);
	lua_pushboolean(L, query->isExplain());
	return 1;
}

/*
 * ok,err = query:max_key(lua_table or json_str)
 */
static int query_max_key(lua_State* L)
{
	mongo::Query* query = userdata_to_query(L, 1);

	try
	{
		int type = lua_type(L, 2);
		if (type == LUA_TSTRING)
		{
			const char* jsonstr = luaL_checkstring(L, 2);
			query->maxKey(mongo::fromjson(jsonstr));
		}
		else if (type == LUA_TTABLE)
		{
			mongo::BSONObj data;
			lua_to_bson(L, 2, data);
			query->maxKey(data);
		}
		else
		{
			throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
		}
	}
	catch (std::exception& e)
	{
		lua_pushboolean(L, 0);
		lua_pushfstring(L, LUAMONGO_ERR_QUERY_FAILED, e.what());
		return 2;
	}
	catch (const char* err)
	{
		lua_pushboolean(L, 0);
		lua_pushstring(L, err);
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

/*
 * ok,err = query:min_key(lua_table or json_str)
 */
static int query_min_key(lua_State* L)
{
	mongo::Query* query = userdata_to_query(L, 1);

	try
	{
		int type = lua_type(L, 2);
		if (type == LUA_TSTRING)
		{
			const char* jsonstr = luaL_checkstring(L, 2);
			query->minKey(mongo::fromjson(jsonstr));
		}
		else if (type == LUA_TTABLE)
		{
			mongo::BSONObj data;
			lua_to_bson(L, 2, data);
			query->minKey(data);
		}
		else
		{
			throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
		}
	}
	catch (std::exception& e)
	{
		lua_pushboolean(L, 0);
		lua_pushfstring(L, LUAMONGO_ERR_QUERY_FAILED, e.what());
		return 2;
	}
	catch (const char* err)
	{
		lua_pushboolean(L, 0);
		lua_pushstring(L, err);
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

/*
 * ok,err = query:snapshot()
 */
static int query_snapshot(lua_State* L)
{
	mongo::Query* query = userdata_to_query(L, 1);

	try
	{
		query->snapshot();
	}
	catch (std::exception& e)
	{
		lua_pushboolean(L, 0);
		lua_pushfstring(L, LUAMONGO_ERR_QUERY_FAILED, e.what());
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

/*
 * ok,err = query:sort(lua_table or json_str)
 * ok,err = query:sort(fieldname, sort_ascending)
 */
static int query_sort(lua_State* L)
{
	int n = lua_gettop(L);
	mongo::Query* query = userdata_to_query(L, 1);

	if (n >= 3)
	{
		const char* field = luaL_checkstring(L, 2);
		int asc = lua_toboolean(L, 3) != 0 ? 1 : -1;

		try
		{
			query->sort(field, asc);
		}
		catch (std::exception& e)
		{
			lua_pushboolean(L, 0);
			lua_pushfstring(L, LUAMONGO_ERR_QUERY_FAILED, e.what());
			return 2;
		}
	}
	else
	{
		try
		{
			int type = lua_type(L, 2);
			if (type == LUA_TSTRING)
			{
				const char* jsonstr = luaL_checkstring(L, 2);
				query->sort(mongo::fromjson(jsonstr));
			}
			else if (type == LUA_TTABLE)
			{
				mongo::BSONObj data;
				lua_to_bson(L, 2, data);
				query->sort(data);
			}
			else
			{
				throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
			}
		}
		catch (std::exception& e)
		{
			lua_pushboolean(L, 0);
			lua_pushfstring(L, LUAMONGO_ERR_QUERY_FAILED, e.what());
			return 2;
		}
		catch (const char* err)
		{
			lua_pushboolean(L, 0);
			lua_pushstring(L, err);
			return 2;
		}
	}

	lua_pushboolean(L, 1);
	return 1;
}

/*
 * ok, err = query:where(jscode)
 * ok, err = query:where(jscode, lua_table or json_str)
 */
static int query_where(lua_State* L)
{
	int n = lua_gettop(L);
	mongo::Query* query = userdata_to_query(L, 1);
	std::string jscode = luaL_checkstring(L, 2);
	if (n > 2)
	{
		mongo::BSONObj scope;
		try
		{
			int type = lua_type(L, 3);
			if (type == LUA_TSTRING)
			{
				const char* jsonstr = luaL_checkstring(L, 3);
				scope = mongo::fromjson(jsonstr);
			}
			else if (type == LUA_TTABLE)
			{
				lua_to_bson(L, 3, scope);
			}
			else
			{
				throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
			}
			query->where(jscode, scope);
		}
		catch (std::exception& e)
		{
			lua_pushboolean(L, 0);
			lua_pushfstring(L, LUAMONGO_ERR_QUERY_FAILED, e.what());
			return 2;
		}
		catch (const char* err)
		{
			lua_pushnil(L);
			lua_pushstring(L, err);
			return 2;
		}
	}
	else
	{
		try
		{
			query->where(jscode);
		}
		catch (std::exception& e)
		{
			lua_pushboolean(L, 0);
			lua_pushfstring(L, LUAMONGO_ERR_QUERY_FAILED, e.what());
			return 2;
		}
	}

	lua_pushboolean(L, 1);
	return 1;
}

/*
 * __gc
 */
static int query_gc(lua_State* L)
{
	mongo::Query* query = userdata_to_query(L, 1);
	delete query;
	return 0;
}

/*
 * __tostring
 */
static int query_tostring(lua_State* L)
{
	void* ud = 0;

	ud = luaL_checkudata(L, 1, LUAMONGO_QUERY);
	mongo::Query* query = *((mongo::Query**)ud);

	lua_pushstring(L, query->toString().c_str());

	return 1;
}

int mongo_query_register(lua_State* L)
{
	static const luaL_Reg query_methods[] =
	{
		{"explain", query_explain},
		{"hint", query_hint},
		{"is_explain", query_is_explain},
		{"max_key", query_max_key},
		{"min_key", query_min_key},
		{"snapshot", query_snapshot},
		{"sort", query_sort},
		{"where", query_where},
		{NULL, NULL}
	};

	static const luaL_Reg query_class_methods[] =
	{
		{"New", query_new},
		{NULL, NULL}
	};

	luaL_newmetatable(L, LUAMONGO_QUERY);
	luaL_register(L, 0, query_methods);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	lua_pushcfunction(L, query_gc);
	lua_setfield(L, -2, "__gc");

	lua_pushcfunction(L, query_tostring);
	lua_setfield(L, -2, "__tostring");

	luaL_register(L, LUAMONGO_QUERY, query_class_methods);

	lua_pushstring(L, "Options");
	lua_newtable(L);

	lua_pushstring(L, "CursorTailable");
	lua_pushinteger(L, mongo::QueryOption_CursorTailable);
	lua_rawset(L, -3);

	lua_pushstring(L, "SlaveOk");
	lua_pushinteger(L, mongo::QueryOption_SlaveOk);
	lua_rawset(L, -3);

	lua_pushstring(L, "OplogReplay");
	lua_pushinteger(L, mongo::QueryOption_OplogReplay);
	lua_rawset(L, -3);

	lua_pushstring(L, "NoCursorTimeout");
	lua_pushinteger(L, mongo::QueryOption_NoCursorTimeout);
	lua_rawset(L, -3);

	lua_pushstring(L, "AwaitData");
	lua_pushinteger(L, mongo::QueryOption_AwaitData);
	lua_rawset(L, -3);

	lua_rawset(L, -3);

	return 1;
}
