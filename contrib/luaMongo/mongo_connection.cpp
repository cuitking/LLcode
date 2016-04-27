#include "common.h"
#include "utils.h"
#include "DBEvent.h"
#include "DBServer.h"

#include <objbase.h>
#include <stdio.h>
#include <boost/pool/object_pool.hpp>

typedef boost::object_pool<CDBQueryEvent>						QueryEventPool;
typedef boost::object_pool<CDBInsertEvent>						InsertEventPool;
typedef boost::object_pool<CDBUpdateEvent>						UpdateEventPool;
typedef boost::object_pool<CDBRemoveEvent>						RemoveEventPool;
typedef boost::object_pool<CDBCommandEvent>						CommandEventPool;
typedef boost::object_pool<CDBCountEvent>						CountEventPool;
static QueryEventPool	queryEventPool;
static InsertEventPool	insertEventPool;
static UpdateEventPool	updateEventPool;
static RemoveEventPool	removeEventPool;
static CommandEventPool	commandEventPool;
static CountEventPool	countEventPool;

extern void lua_to_bson(lua_State* L, int stackpos, mongo::BSONObj& obj);
extern void bson_to_lua(lua_State* L, const mongo::BSONObj& obj);
extern void lua_push_value(lua_State* L, const mongo::BSONElement& elem);

namespace
{
	inline mongo::DBClientConnection* userdata_to_connection(lua_State* L, int index)
	{
		void* ud = 0;
		ud = luaL_checkudata(L, index, LUAMONGO_CONNECTION);
		mongo::DBClientConnection* connection = *((mongo::DBClientConnection**)ud);
		return connection;
	}
} // anonymous namespace

/*
* db,err = mongo.Connection.New({})
*    accepts an optional table of features:
*       auto_reconnect   (default = false)
*       rw_timeout       (default = 0) (mongo >= v1.5)
*/
static int connection_new(lua_State* L)
{
	int resultcount = 1;
	try
	{
		bool auto_reconnect = false;
		int rw_timeout = 0;
		if (lua_type(L, 1) == LUA_TTABLE)
		{
			lua_getfield(L, 1, "auto_reconnect");
			auto_reconnect = lua_toboolean(L, -1) != 0;
			lua_getfield(L, 1, "rw_timeout");
			rw_timeout = luaL_optint(L, -1, 0);
			lua_pop(L, 2);
		}
		mongo::DBClientConnection** connection = (mongo::DBClientConnection**)lua_newuserdata(L, sizeof(mongo::DBClientConnection*));
		*connection = new mongo::DBClientConnection(auto_reconnect, 0, rw_timeout);
		SDBServer::Instance().AddConnection(*connection);
		luaL_getmetatable(L, LUAMONGO_CONNECTION);
		lua_setmetatable(L, -2);
	}
	catch (std::exception& e)
	{
		lua_pushnil(L);
		lua_pushfstring(L, LUAMONGO_ERR_CONNECTION_FAILED, e.what());
		resultcount = 2;
	}
	return resultcount;
}

/*
* ok,err = db:connect(connection_str)
*/
static int connection_connect(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	const char* connectstr = luaL_checkstring(L, 2);

	try
	{
		connection->connect(connectstr);
	}
	catch (std::exception& e)
	{
		lua_pushnil(L);
		lua_pushfstring(L, LUAMONGO_ERR_CONNECT_FAILED, connectstr, e.what());
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

/*
* created = db:ensure_index(ns, json_str or lua_table[, unique[, name]])
*/
static int connection_ensure_index(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	const char* ns = luaL_checkstring(L, 2);
	mongo::BSONObj fields;

	try
	{
		int type = lua_type(L, 3);
		if (type == LUA_TSTRING)
		{
			size_t len;
			const char *s = lua_tolstring(L, 3, &len);
			fields = mongo::fromjson(s, (int*)&len);
		}
		else if (type == LUA_TTABLE)
		{
			lua_to_bson(L, 3, fields);
		}
		else
		{
			throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
		}
	}
	catch (std::exception& e)
	{
		lua_pushboolean(L, 0);
		lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "ensure_index", e.what());
		return 2;
	}
	catch (const char* err)
	{
		lua_pushboolean(L, 0);
		lua_pushstring(L, err);
		return 2;
	}

	bool unique = lua_toboolean(L, 4) != 0;
	const char* name = luaL_optstring(L, 5, "");

	bool res = connection->ensureIndex(ns, fields, unique, name);

	lua_pushboolean(L, res);
	return 1;
}

/*
* ok,err = db:auth({})
*    accepts a table of parameters:
*       dbname           database to authenticate (required)
*       username         username to authenticate against (required)
*       password         password to authenticate against (required)
*       digestPassword   set to true if password is pre-digested (default = true)
*
*/
static int connection_auth(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);

	luaL_checktype(L, 2, LUA_TTABLE);
	lua_getfield(L, 2, "dbname");
	const char* dbname = luaL_checkstring(L, -1);
	lua_getfield(L, 2, "username");
	const char* username = luaL_checkstring(L, -1);
	lua_getfield(L, 2, "password");
	const char* password = luaL_checkstring(L, -1);
	lua_getfield(L, 2, "digestPassword");
	bool digestPassword = lua_isnil(L, -1) ? true : lua_toboolean(L, -1) != 0;
	lua_pop(L, 4);

	std::string errmsg;
	bool success = connection->auth(dbname, username, password, errmsg, digestPassword);
	if (!success)
	{
		lua_pushnil(L);
		lua_pushfstring(L, LUAMONGO_ERR_CONNECTION_FAILED, errmsg.c_str());
		return 2;
	}
	lua_pushboolean(L, 1);
	return 1;
}

/*
* is_failed = db:is_failed()
*/
static int connection_is_failed(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);

	bool is_failed = connection->isFailed();
	lua_pushboolean(L, is_failed);
	return 1;
}

/*
* addr = db:get_server_address()
*/
static int connection_get_server_address(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	std::string address = connection->getServerAddress();
	lua_pushlstring(L, address.c_str(), address.length());
	return 1;
}

void Handle_Write(CDBEvent* db)
{
	lua_State* L = db->GetLuaState();
	int n = lua_gettop(L);
	mongo::BSONObj retInfo;
	if (db->GetResult(retInfo))
	{
		bson_to_lua(L, retInfo);
	}
	else
	{
		lua_pushnil(L);
	}
	db->Clear();
	try
	{
		int r = lua_resume(L, lua_gettop(L) - n);
		if (r != LUA_YIELD && r != 0)
		{
			std::cout << lua_tostring(L, -1) << std::endl;
		}
		lua_pop(L, lua_gettop(L) - n);
	}
	catch(...)
	{
		std::cout << "Handle_Write Exception \n";
	}
}

void Handle_Insert(ULONG_PTR dwParam)
{
	CDBInsertEvent* db = reinterpret_cast<CDBInsertEvent*>(dwParam);
	if (db == NULL)
	{
		return;
	}
	Handle_Write(db);
	insertEventPool.destroy(db);
}

/*
* ok,err = db:insert(ns, lua_table or json_str)
*/
static int connection_insert(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	const char* ns = luaL_checkstring(L, 2);
	mongo::BSONObj* obj = new mongo::BSONObj;
	try
	{
		int type = lua_type(L, 3);
		if (type == LUA_TSTRING)
		{
			size_t len;
			const char *s = lua_tolstring(L, 3, &len);
			*obj = mongo::fromjson(s, (int*)&len);
		}
		else if (type == LUA_TTABLE)
		{
			mongo::BSONObj data;
			lua_to_bson(L, 3, *obj);
		}
		else
		{
			throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
		}
		int writeConcern = -1;
		if (lua_isnumber(L, 4))
		{
			writeConcern = lua_tonumber(L, 4);
		}
		CDBInsertEvent* db = insertEventPool.construct();
		db->SetValue(connection, ns, obj, writeConcern, L);
		SDBServer::Instance().AsyncProcess(connection, db);
		lua_pushboolean(L, 1);
		return 1;
	}
	catch (std::exception& e)
	{
		delete obj;
		lua_pushboolean(L, 0);
		lua_pushfstring(L, LUAMONGO_ERR_INSERT_FAILED, e.what());
		return 2;
	}
	catch (const char* err)
	{
		delete obj;
		lua_pushboolean(L, 0);
		lua_pushstring(L, err);
		return 2;
	}
	catch (...)
	{
		delete obj;
		lua_pushboolean(L, 0);
		return 1;
	}
	return 0;
}

/*
* ok,err = db:insert_batch(ns, lua_array_of_tables)
*/
static int connection_insert_batch(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	const char* ns = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);

	try
	{
		std::vector<mongo::BSONObj> vdata;
		size_t tlen = lua_objlen(L, 3) + 1;
		for (size_t i = 1; i < tlen; ++i)
		{
			vdata.push_back(mongo::BSONObj());
			lua_rawgeti(L, 3, i);
			lua_to_bson(L, 4, vdata.back());
			lua_pop(L, 1);
		}
		connection->insert(ns, vdata);
	}
	catch (std::exception& e)
	{
		lua_pushboolean(L, 0);
		lua_pushfstring(L, LUAMONGO_ERR_INSERT_FAILED, e.what());
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

void PushResultToLua(lua_State* L, CDBQueryEvent* db)
{
	std::vector<mongo::BSONObj>& results = db->GetResult();
	lua_newtable(L);
	int count = 0;
	for (std::vector<mongo::BSONObj>::iterator iter = results.begin(); iter != results.end(); ++iter)
	{
		const mongo::BSONObj& obj = *iter;
		count = count + 1;
		lua_pushnumber(L, count);
		bson_to_lua(L, obj);
		lua_rawset(L, -3);
	}
}

void Handle_Query(ULONG_PTR dwParam)
{
	CDBQueryEvent* db = reinterpret_cast<CDBQueryEvent*>(dwParam);
	if (db == NULL)
	{
		return;
	}
	__try
	{
		lua_State* L = db->GetLuaState();
		int n = lua_gettop(L);
		PushResultToLua(L, db);
		db->Clear();
		queryEventPool.destroy(db);
		int r = lua_resume(L, lua_gettop(L) - n);
		if (r != LUA_YIELD && r != 0)
		{
			std::cout << lua_tostring(L, -1) << std::endl;
		}
		lua_pop(L, lua_gettop(L) - n);
	}
	__except (GetExceptionCode())
	{
		std::cout << "Handle_Query Exception " << GetExceptionCode() << "\n";
	}
}

/*
* cursor,err = db:query(ns, lua_table or json_str or query_obj, limit, skip, lua_table or json_str, options, batchsize)
*/
static int connection_query(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	const char* ns = luaL_checkstring(L, 2);
	mongo::Query* query = new mongo::Query;
	if (!lua_isnoneornil(L, 3))
	{
		try
		{
			int type = lua_type(L, 3);
			if (type == LUA_TSTRING)
			{
				size_t len;
				const char *s = lua_tolstring(L, 3, &len);
				*query = mongo::fromjson(s, (int*)&len);
			}
			else if (type == LUA_TTABLE)
			{
				mongo::BSONObj obj;
				lua_to_bson(L, 3, obj);
				*query = obj;
			}
			else if (type == LUA_TUSERDATA)
			{
				void* uq = 0;
				uq = luaL_checkudata(L, 3, LUAMONGO_QUERY);
				*query = *(*((mongo::Query**)uq));
			}
			else
			{
				throw(LUAMONGO_REQUIRES_QUERY);
			}
		}
		catch (std::exception& e)
		{
			delete query;
			lua_pushnil(L);
			lua_pushfstring(L, LUAMONGO_ERR_QUERY_FAILED, e.what());
			return 2;
		}
		catch (const char* err)
		{
			delete query;
			lua_pushnil(L);
			lua_pushstring(L, err);
			return 2;
		}
		catch (...)
		{
			delete query;
			lua_pushnil(L);
			return 1;
		}
	}
	
	mongo::BSONObj* fieldsToReturn = NULL;
	if (!lua_isnoneornil(L, 4))
	{
		fieldsToReturn = new mongo::BSONObj;
		try
		{
			int type = lua_type(L, 4);
			if (type == LUA_TSTRING)
			{
				size_t len;
				const char *s = lua_tolstring(L, 4, &len);
				*fieldsToReturn = mongo::fromjson(s, (int*)&len);
			}
			else if (type == LUA_TTABLE)
			{
				lua_to_bson(L, 4, *fieldsToReturn);
			}
			else
			{
				throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
			}
		}
		catch (std::exception& e)
		{
			delete query;
			delete fieldsToReturn;
			lua_pushnil(L);
			lua_pushfstring(L, LUAMONGO_ERR_QUERY_FAILED, e.what());
			return 2;
		}
		catch (const char* err)
		{
			delete query;
			delete fieldsToReturn;
			lua_pushnil(L);
			lua_pushstring(L, err);
			return 2;
		}
		catch (...)
		{
			delete query;
			delete fieldsToReturn;
			lua_pushnil(L);
			return 1;
		}
	}

	int nToReturn = luaL_optint(L, 5, 0);
	int nToSkip = luaL_optint(L, 6, 0);
	bool nonBlock = lua_toboolean(L, 7) != 0;

	CDBQueryEvent* db = queryEventPool.construct();
	HANDLE event = INVALID_HANDLE_VALUE;
	if (!nonBlock) event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	db->SetValue(connection, ns, query, nToReturn, nToSkip, 
		fieldsToReturn, nonBlock, (int)event, L);
	SDBServer::Instance().AsyncProcess(connection, db);
	lua_pushboolean(L, 1);

	if (nonBlock) return 1;
	::WaitForSingleObject(event, INFINITE);
	CloseHandle(event);
	PushResultToLua(L, db);
	db->Clear();
	queryEventPool.destroy(db);
	return 2;
}

void Handle_Remove(ULONG_PTR dwParam)
{
	CDBRemoveEvent* db = reinterpret_cast<CDBRemoveEvent*>(dwParam);
	if (db == NULL)
	{
		return;
	}
	Handle_Write(db);
	removeEventPool.destroy(db);
}

/*
* ok,err = db:remove(ns, lua_table or json_str or query_obj)
*/
static int connection_remove(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	const char* ns = luaL_checkstring(L, 2);
	mongo::Query* query = new mongo::Query;
	try
	{
		int type = lua_type(L, 3);
		if (type == LUA_TSTRING)
		{
			size_t len;
			const char *s = lua_tolstring(L, 3, &len);
			*query = mongo::fromjson(s, (int*)&len);
		}
		else if (type == LUA_TTABLE)
		{
			mongo::BSONObj obj;
			lua_to_bson(L, 3, obj);
			*query = obj;
		}
		else if (type == LUA_TUSERDATA)
		{
			void* uq = 0;
			uq = luaL_checkudata(L, 3, LUAMONGO_QUERY);
			*query = *(*((mongo::Query**)uq));
		}
		else
		{
			throw(LUAMONGO_REQUIRES_QUERY);
		}

		
		bool justOne = false;
		if (lua_isboolean(L, 4))
		{
			justOne = lua_toboolean(L, 4) != 0;
		}
		int writeConcern = -1;
		if (lua_isnumber(L, 5))
		{
			writeConcern = lua_tonumber(L, 5);
		}

		CDBRemoveEvent* db = removeEventPool.construct();
		db->SetValue(connection, ns, query, justOne, writeConcern, L);
		SDBServer::Instance().AsyncProcess(connection, db);
	}
	catch (std::exception& e)
	{
		delete query;
		lua_pushboolean(L, 0);
		lua_pushfstring(L, LUAMONGO_ERR_REMOVE_FAILED, e.what());
		return 2;
	}
	catch (const char* err)
	{
		delete query;
		lua_pushboolean(L, 0);
		lua_pushstring(L, err);
		return 2;
	}
	catch (...)
	{
		delete query;
		lua_pushboolean(L, 0);
		return 1;
	}

	lua_pushboolean(L, 1);
	return 1;
}

void Handle_Update(ULONG_PTR dwParam)
{
	CDBUpdateEvent* db = reinterpret_cast<CDBUpdateEvent*>(dwParam);
	if (db == NULL)
	{
		return;
	}
	Handle_Write(db);
	updateEventPool.destroy(db);
}

/*
* ok,err = db:update(ns, lua_table or json_str or query_obj, lua_table or json_str, upsert, multi)
*/
static int connection_update(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	const char* ns = luaL_checkstring(L, 2);

	mongo::Query* query = new mongo::Query;
	mongo::BSONObj* obj = new mongo::BSONObj;

	try
	{
		int type_query = lua_type(L, 3);
		if (type_query == LUA_TSTRING)
		{
			size_t len;
			const char *s = lua_tolstring(L, 3, &len);
			*query = mongo::fromjson(s, (int*)&len);
		}
		else if (type_query == LUA_TTABLE)
		{
			mongo::BSONObj q;
			lua_to_bson(L, 3, q);
			*query = q;
		}
		else if (type_query == LUA_TUSERDATA)
		{
			void* uq = 0;
			uq = luaL_checkudata(L, 3, LUAMONGO_QUERY);
			*query = *(*((mongo::Query**)uq));
		}
		else
		{
			throw(LUAMONGO_REQUIRES_QUERY);
		}

		int type_obj = lua_type(L, 4);
		if (type_obj == LUA_TSTRING)
		{
			size_t len;
			const char *s = lua_tolstring(L, 4, &len);
			*obj = mongo::fromjson(s, (int*)&len);
		}
		else if (type_obj == LUA_TTABLE)
		{
			lua_to_bson(L, 4, *obj);
		}
		else
		{
			throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
		}
	}
	catch (std::exception& e)
	{
		delete query;
		delete obj;
		lua_pushboolean(L, 0);
		lua_pushfstring(L, LUAMONGO_ERR_UPDATE_FAILED, e.what());
		return 2;
	}
	catch (const char* err)
	{
		delete query;
		delete obj;
		lua_pushboolean(L, 0);
		lua_pushstring(L, err);
		return 2;
	}
	catch (...)
	{
		delete query;
		delete obj;
		lua_pushboolean(L, 0);
		return 1;
	}

	bool upsert = false;
	if (lua_isboolean(L, 5))
	{
		upsert = lua_toboolean(L, 5) != 0;
	}
	bool multi = false;
	if (lua_isboolean(L, 6))
	{
		multi = lua_toboolean(L, 6) != 0;
	}
	int writeConcern = -1;
	if (lua_isnumber(L, 7))
	{
		writeConcern = lua_tonumber(L, 7);
	}

	CDBUpdateEvent* db = updateEventPool.construct();
	db->SetValue(connection, ns, query, obj, upsert, multi, writeConcern, L);
	SDBServer::Instance().AsyncProcess(connection, db);

	lua_pushboolean(L, 1);
	return 1;
}

/*
* ok,err = db:drop_collection(ns)
*/
static int connection_drop_collection(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	const char* ns = luaL_checkstring(L, 2);

	try
	{
		connection->dropCollection(ns);
	}
	catch (std::exception& e)
	{
		lua_pushboolean(L, 0);
		lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "drop_collection", e.what());
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

/*
* ok,err = db:drop_index_by_fields(ns, json_str or lua_table)
*/
static int connection_drop_index_by_fields(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	const char* ns = luaL_checkstring(L, 2);

	mongo::BSONObj keys;

	try
	{
		int type = lua_type(L, 3);
		if (type == LUA_TSTRING)
		{
			size_t len;
			const char *s = lua_tolstring(L, 3, &len);
			keys = mongo::fromjson(s, (int*)&len);
		}
		else if (type == LUA_TTABLE)
		{
			lua_to_bson(L, 3, keys);
		}
		else
		{
			throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
		}

		connection->dropIndex(ns, keys);
	}
	catch (std::exception& e)
	{
		lua_pushboolean(L, 0);
		lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "drop_index_by_fields", e.what());
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
* ok,err = db:drop_index_by_name(ns, index_name)
*/
static int connection_drop_index_by_name(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	const char* ns = luaL_checkstring(L, 2);

	try
	{
		connection->dropIndex(ns, luaL_checkstring(L, 3));
	}
	catch (std::exception& e)
	{
		lua_pushboolean(L, 0);
		lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "drop_index_by_name", e.what());
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

/*
* ok,err = db:drop_indexes(ns)
*/
static int connection_drop_indexes(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	const char* ns = luaL_checkstring(L, 2);

	try
	{
		connection->dropIndexes(ns);
	}
	catch (std::exception& e)
	{
		lua_pushboolean(L, 0);
		lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "drop_indexes", e.what());
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

/*
* res,err = (dbname, jscode[, args_table])
*/
static int connection_eval(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	const char* dbname = luaL_checkstring(L, 2);
	const char* jscode = luaL_checkstring(L, 3);

	mongo::BSONObj info;
	mongo::BSONElement retval;
	mongo::BSONObj* args = NULL;
	if (!lua_isnoneornil(L, 4))
	{
		try
		{
			int type = lua_type(L, 4);

			if (type == LUA_TSTRING)
			{
				args = new mongo::BSONObj(luaL_checkstring(L, 4));
			}
			else if (type == LUA_TTABLE)
			{
				mongo::BSONObj obj;
				lua_to_bson(L, 4, obj);

				args = new mongo::BSONObj(obj);
			}
			else
			{
				throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
			}
		}
		catch (std::exception& e)
		{
			lua_pushnil(L);
			lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "eval", e.what());
			return 2;
		}
		catch (const char* err)
		{
			lua_pushnil(L);
			lua_pushstring(L, err);
			return 2;
		}
	}

	bool res = connection->eval(dbname, jscode, info, retval, args);

	if (!res)
	{
		lua_pushboolean(L, 0);
		lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "eval", info["errmsg"].str().c_str());

		return 2;
	}

	if (args)
	{
		delete args;
	}

	lua_push_value(L, retval);
	return 1;
}

/*
* bool = db:exists(ns)
*/
static int connection_exists(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	const char* ns = luaL_checkstring(L, 2);

	bool res = connection->exists(ns);

	lua_pushboolean(L, res);

	return 1;
}

/*
* name = db:gen_index_name(json_str or lua_table)
*/
static int connection_gen_index_name(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	std::string name = "";

	try
	{
		int type = lua_type(L, 2);
		if (type == LUA_TSTRING)
		{
			size_t len;
			const char *s = lua_tolstring(L, 2, &len);
			mongo::BSONObj obj = mongo::fromjson(s, (int*)&len);
			name = connection->genIndexName(obj);
		}
		else if (type == LUA_TTABLE)
		{
			mongo::BSONObj data;
			lua_to_bson(L, 2, data);

			name = connection->genIndexName(data);
		}
		else
		{
			throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
		}
	}
	catch (std::exception& e)
	{
		lua_pushnil(L);
		lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "gen_index_name", e.what());
		return 2;
	}
	catch (const char* err)
	{
		lua_pushnil(L);
		lua_pushstring(L, err);
		return 2;
	}

	lua_pushstring(L, name.c_str());
	return 1;
}

/*
* cursor = db:get_indexes(ns)
*/
static int connection_get_indexes(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	const char* ns = luaL_checkstring(L, 2);

	std::auto_ptr<mongo::DBClientCursor> autocursor = connection->getIndexes(ns);

	mongo::DBClientCursor** cursor = (mongo::DBClientCursor**)lua_newuserdata(L, sizeof(mongo::DBClientCursor*));
	*cursor = autocursor.get();
	autocursor.release();

	luaL_getmetatable(L, LUAMONGO_CURSOR);
	lua_setmetatable(L, -2);

	return 1;
}

/*
* res,err = db:mapreduce(jsmapfunc, jsreducefunc[, query[, output]])
*/
static int connection_mapreduce(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	const char* ns = luaL_checkstring(L, 2);
	const char* jsmapfunc = luaL_checkstring(L, 3);
	const char* jsreducefunc = luaL_checkstring(L, 4);

	mongo::BSONObj query;
	if (!lua_isnoneornil(L, 5))
	{
		try
		{
			int type = lua_type(L, 5);
			if (type == LUA_TSTRING)
			{
				size_t len;
				const char *s = lua_tolstring(L, 5, &len);
				query = mongo::fromjson(s, (int*)&len);
			}
			else if (type == LUA_TTABLE)
			{
				lua_to_bson(L, 5, query);
			}
			else
			{
				throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
			}
		}
		catch (std::exception& e)
		{
			lua_pushnil(L);
			lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "mapreduce", e.what());
			return 2;
		}
		catch (const char* err)
		{
			lua_pushnil(L);
			lua_pushstring(L, err);
			return 2;
		}
	}

	const char* output = luaL_optstring(L, 6, "");
	mongo::BSONObj res = connection->mapreduce(ns, jsmapfunc, jsreducefunc, query, output);
	bson_to_lua(L, res);
	return 1;
}

/*
* ok,err = db:reindex(ns);
*/
static int connection_reindex(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	const char* ns = luaL_checkstring(L, 2);

	try
	{
		connection->reIndex(ns);
	}
	catch (std::exception& e)
	{
		lua_pushboolean(L, 0);
		lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "reindex", e.what());
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

void Handle_Command(ULONG_PTR dwParam)
{
	CDBCommandEvent* db = reinterpret_cast<CDBCommandEvent*>(dwParam);
	if (db == NULL)
	{
		return;
	}
	try
	{
		lua_State* L = db->GetLuaState();
		int n = lua_gettop(L);
		mongo::BSONObj retInfo;
		if (db->GetResult(retInfo))
		{
			bson_to_lua(L, retInfo);
		}
		else
		{
			lua_pushnil(L);
		}
		db->Clear();
		commandEventPool.destroy(db);
		int r = lua_resume(L, lua_gettop(L) - n);
		if (r != LUA_YIELD && r != 0)
		{
			std::cout << lua_tostring(L, -1) << std::endl;
		}
		lua_pop(L, lua_gettop(L) - n);
	}
	catch(...)
	{
		std::cout << "Handle_Command Exception \n";
	}
}

/*
* count,err = db:runcommand(ns)
*/
static int connection_runcommand(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	const char* ns = luaL_checkstring(L, 2);
	mongo::BSONObj *obj = new mongo::BSONObj;

	try
	{
		if (!lua_isnoneornil(L, 3))
		{
			int type = lua_type(L, 3);
			if (type == LUA_TSTRING)
			{
				size_t len;
				const char *s = lua_tolstring(L, 3, &len);
				*obj = mongo::fromjson(s, (int*)&len);
			}
			else if (type == LUA_TTABLE)
			{
				lua_to_bson(L, 3, *obj);
			}
			else
			{
				luaL_error(L, LUAMONGO_REQUIRES_JSON_OR_TABLE);
			}
		}
		else
		{
			delete obj;
			return 0;
		}
	}
	catch (std::exception& e)
	{
		delete obj;
		lua_pushnil(L);
		lua_pushfstring(L, LUAMONGO_ERR_QUERY_FAILED, e.what());
		return 2;
	}
	catch (const char* err)
	{
		delete obj;
		lua_pushnil(L);
		lua_pushstring(L, err);
		return 2;
	}
	catch (...)
	{
		delete obj;
		lua_pushnil(L);
		return 1;
	}

	CDBCommandEvent* db = commandEventPool.construct();
	db->SetValue(connection, ns, obj, L);
	SDBServer::Instance().AsyncProcess(connection, db);
	lua_pushboolean(L, 1);
	return 1;
}

void Handle_Count(ULONG_PTR dwParam)
{
	CDBCountEvent* db = reinterpret_cast<CDBCountEvent*>(dwParam);
	if (db == NULL)
	{
		return;
	}
	lua_State* L = db->GetLuaState();
	int n = lua_gettop(L);
	__try
	{
		lua_pushnumber(L, db->GetResult());
		db->Clear();
		countEventPool.destroy(db);
		int r = lua_resume(L, lua_gettop(L) - n);
		if (r != LUA_YIELD && r != 0)
		{
			std::cout << lua_tostring(L, -1) << std::endl;
		}
		lua_pop(L, lua_gettop(L) - n);
	}
	__except (GetExceptionCode())
	{
		std::cout << "Handle_Count Exception " << GetExceptionCode() << "\n";
	}
}


/*
* count,err = db:count(ns)
*/
static int connection_count(lua_State* L)
{
	int n = lua_gettop(L);
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	const char* ns = luaL_checkstring(L, 2);
	mongo::BSONObj* obj = new mongo::BSONObj;
	if (!lua_isnoneornil(L, 3))
	{
		try
		{
			int type = lua_type(L, 3);
			if (type == LUA_TSTRING)
			{
				size_t len;
				const char *s = lua_tolstring(L, 3, &len);
				*obj = mongo::fromjson(s, (int*)&len);
			}
			else if (type == LUA_TTABLE)
			{
				lua_to_bson(L, 3, *obj);
			}
			else
			{
				luaL_error(L, LUAMONGO_REQUIRES_QUERY);
			}
		}
		catch (std::exception& e)
		{
			delete obj;
			lua_pushboolean(L, FALSE);
			lua_pushfstring(L, LUAMONGO_ERR_QUERY_FAILED, e.what());
			return 2;
		}
		catch (const char* err)
		{
			delete obj;
			lua_pushboolean(L, FALSE);
			lua_pushstring(L, err);
			return 2;
		}
		catch (...)
		{
			delete obj;
			lua_pushnil(L);
			return 1;
		}
	}

	int nToReturn = luaL_optint(L, 4, 0);
	int nToSkip = luaL_optint(L, 5, 0);

	CDBCountEvent* db = countEventPool.construct();
	db->SetValue(connection, ns, obj, nToReturn, nToSkip, L);
	SDBServer::Instance().AsyncProcess(connection, db);
	lua_pushboolean(L, 1);
	return 1;
}

/*
* db:reset_index_cache()
*/
static int connection_reset_index_cache(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);

	connection->resetIndexCache();

	return 0;
}

/*
* __gc
*/
static int connection_gc(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	delete connection;
	return 0;
}

/*
* __tostring
*/
static int connection_tostring(lua_State* L)
{
	mongo::DBClientConnection* connection = userdata_to_connection(L, 1);
	lua_pushfstring(L, "%s: %s", LUAMONGO_CONNECTION,  connection->toString().c_str());

	return 1;
}


int mongo_connection_register(lua_State* L)
{
	static const luaL_Reg connection_methods[] =
	{
		{"auth", connection_auth},
		{"connect", connection_connect},
		{"count", connection_count},
		{"drop_collection", connection_drop_collection},
		{"drop_index_by_fields", connection_drop_index_by_fields},
		{"drop_index_by_name", connection_drop_index_by_name},
		{"drop_indexes", connection_drop_indexes},
		{"ensure_index", connection_ensure_index},
		{"eval", connection_eval},
		{"exists", connection_exists},
		{"gen_index_name", connection_gen_index_name},
		{"get_indexes", connection_get_indexes},
		{"get_server_address", connection_get_server_address},
		{"insert", connection_insert},
		{"insert_batch", connection_insert_batch},
		{"is_failed", connection_is_failed},
		{"mapreduce", connection_mapreduce},
		{"query", connection_query},
		{"reindex", connection_reindex},
		{"remove", connection_remove},
		{"reset_index_cache", connection_reset_index_cache},
		{"update", connection_update},
		{"runcommand", connection_runcommand},
		{NULL, NULL}
	};

	static const luaL_Reg connection_class_methods[] =
	{
		{"New", connection_new},
		{NULL, NULL}
	};

	luaL_newmetatable(L, LUAMONGO_CONNECTION);
	luaL_register(L, 0, connection_methods);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	lua_pushcfunction(L, connection_gc);
	lua_setfield(L, -2, "__gc");

	lua_pushcfunction(L, connection_tostring);
	lua_setfield(L, -2, "__tostring");

	luaL_register(L, LUAMONGO_CONNECTION, connection_class_methods);

	return 1;
}
