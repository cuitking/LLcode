#include "common.h"
#include "utils.h"
#include "DBServer.h"

#include <lua/lua.hpp>
#include <boost/asio.hpp>

extern int mongo_bsontypes_register(lua_State* L);
extern int mongo_connection_register(lua_State* L);
extern int mongo_cursor_register(lua_State* L);
extern int mongo_query_register(lua_State* L);
extern int mongo_gridfs_register(lua_State* L);
extern int mongo_gridfile_register(lua_State* L);
extern int mongo_gridfschunk_register(lua_State* L);

extern boost::asio::io_service* work_service; //业务逻辑线程io_service

static int mongo_init(lua_State* L)
{
	int n = lua_gettop(L);
	work_service = static_cast<boost::asio::io_service*>(lua_touserdata(L, 1));
	bool needLog = lua_toboolean(L, 2) != 0;
	if (needLog)
	{
		//获取本机IP地址
		char host_name[512] = {0};
		if (gethostname(host_name, sizeof(host_name)) == SOCKET_ERROR)
		{
			return 0;
		}

		struct addrinfo hints = {0};
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		struct addrinfo* result = NULL;
		DWORD dwRetval = getaddrinfo(host_name, NULL, &hints, &result);
		if (dwRetval != 0)
		{
			freeaddrinfo(result);
			return lua_gettop(L) - n;
		}

		for (struct addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next)
		{
			struct sockaddr_in*  sockaddr_ipv4 = (struct sockaddr_in*) ptr->ai_addr;
			lua_pushstring(L, inet_ntoa(sockaddr_ipv4->sin_addr));
		}

		freeaddrinfo(result);
		return lua_gettop(L) - n;
	}
	return 0;
}

static int mongo_unInit(lua_State* L)
{
	SDBServer::Instance().UnInit();
	return 0;
}

#ifdef _DEBUG
static int mongo_debug(lua_State* L)
{
	int sleepTime = lua_tonumber(L, -1);
	SDBServer::Instance().SetSleepTime(sleepTime);
	return 0;
}
#endif

static void InitMongo(lua_State* L)
{
	mongo_bsontypes_register(L);
	mongo_connection_register(L);
	mongo_cursor_register(L);
	mongo_query_register(L);

	//mongo_gridfs_register(L);
	//mongo_gridfile_register(L);
	//mongo_gridfschunk_register(L);

	static const luaL_Reg global_methods[] =
	{
		{"init", mongo_init},
		{"uninit", mongo_unInit},
#ifdef _DEBUG
		{"debug", mongo_debug},
#endif	
		{NULL, NULL}
	};

	luaL_register(L, LUAMONGO_ROOT, global_methods);

}

/*
*
* library entry point
*
*/

extern "C" {

#ifdef _DEBUG
	__declspec(dllexport) int luaopen_mongo_d(lua_State* L)
	{
		InitMongo(L);
		/*
		* push the created table to the top of the stack
		* so "mongo = require('mongo_d')" works
		*/
		lua_getglobal(L, LUAMONGO_ROOT);
		return 1;
	}
#else
	__declspec(dllexport) int luaopen_mongo(lua_State* L)
	{
		InitMongo(L);
		/*
		* push the created table to the top of the stack
		* so "mongo = require('mongo')" works
		*/
		lua_getglobal(L, LUAMONGO_ROOT);
		return 1;
	}
#endif


}
