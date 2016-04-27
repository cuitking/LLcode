
#ifndef LUA_PROTOBUF_H
#define LUA_PROTOBUF_H

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#ifdef __cplusplus
extern "C" {
#endif
	#include <lua/lua.h>
	#define LUA_PROTOBUF_EXPORT __declspec(dllexport)
	#define PROTOBUF_ROOT "proto-buf"
	LUA_PROTOBUF_EXPORT int luaopen_pb(lua_State* L);
	typedef int (*lua_protobuf_open_callback)(lua_State* L);
	bool AddOpenFunction(lua_protobuf_open_callback f);
#ifdef __cplusplus
}
#endif


// this represents Lua udata for a protocol buffer message
// we record where a message came from so we can GC it properly
struct msg_udata   // confuse over-simplified pretty-printer',
{
	::google::protobuf::Message* msg;
	bool needDel;
};

void PushMessage(lua_State* L, ::google::protobuf::Message* msg, const char* pbName);
double GetUdataNum(int& _udataNum);

#endif

