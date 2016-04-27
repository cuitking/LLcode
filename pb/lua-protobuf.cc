#include "lua-protobuf.h"
#include <scgl/Win32.h>
#include <boost/filesystem.hpp>
#include <set>

#ifdef __cplusplus
extern "C" {
#endif
#include <lua/lauxlib.h>
#ifdef __cplusplus
}
#endif

//////////////////////////////////////
typedef std::vector<lua_protobuf_open_callback> OpenCallBackVec;
OpenCallBackVec* openlibs = NULL;
int udataNum = 0;
std::set<::google::protobuf::Message*> u_datas;

static void InitProtobuf(lua_State* L)
{
	OpenCallBackVec::iterator iter = openlibs->begin();
	for (; iter != openlibs->end(); ++iter)
	{
		(*iter)(L);
	}
}

bool AddOpenFunction(lua_protobuf_open_callback f)
{
	if (openlibs == NULL)
	{
		openlibs = new OpenCallBackVec;
	}
	openlibs->push_back(f);
	return true;
}

LUA_PROTOBUF_EXPORT int luaopen_pb(lua_State* L)
{
	InitProtobuf(L);
	lua_getglobal(L, PROTOBUF_ROOT);
	return 1;
}

void PushMessage( lua_State* L, ::google::protobuf::Message* msg, const char* pbName )
{
	char metaName[64] = {0};
	sprintf_s(metaName, 64, "%s.%s", PROTOBUF_ROOT, pbName);
	msg_udata *ud = (msg_udata *)lua_newuserdata(L, sizeof(msg_udata));
	ud->msg = msg;
	ud->needDel = true;
	luaL_getmetatable(L, metaName);
	lua_setmetatable(L, -2);
	//udataNum++;
	//u_datas.insert(msg);
}

double GetUdataNum(int& _udataNum)
{
	double memorySize = 0;
	for (std::set<::google::protobuf::Message*>::iterator iter = u_datas.begin(); iter != u_datas.end(); ++iter)
	{
		::google::protobuf::Message* m = *iter;
		memorySize += m->ByteSize();
	}
	_udataNum = udataNum;
	return memorySize/1024;
}
