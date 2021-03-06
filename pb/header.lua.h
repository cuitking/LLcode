// Generated by the lua-protobuf compiler.
// You shouldn't be editing this file manually
//
// source proto file: header.proto
#pragma once

#include "lua-protobuf.h"
#include "header.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <lua/lua.h>

#ifdef __cplusplus
}

// register all messages in this package to a Lua state
static int lua_protobuf_pb_open(lua_State *L);

// Message Header
// registers the message type with Lua
int lua_protobuf_pb_Header_open(lua_State *L);


// push a reference of the message to the Lua stack
// the 3rd and 4th arguments define a callback that can be invoked just before Lua
// garbage collects the message. If the 3rd argument is NULL, Lua will *NOT* free
// memory. If the second argument points to a function, that function is called when
// Lua garbage collects the object. The function is sent a pointer to the message being
// collected and the 4th argument to this function. If the function returns true,
// Lua will free the memory. If false (0), Lua will not free the memory.
void lua_protobuf_pb_Header_(lua_State *L, ::google::protobuf::Message* msg, bool needDel = true);

// The following functions are called by Lua. Many people will not need them,
// but they are exported for those that do.

// constructor called from Lua
int lua_protobuf_pb_Header_new(lua_State *L);

// return operation =
int lua_protobuf_pb_Header_assign(lua_State *L);
// return ::google::Messsage*
int lua_protobuf_pb_Header_data(lua_State *L);

// obtain instance from a serialized string
int lua_protobuf_pb_Header_parsefromstring(lua_State *L);

// garbage collects message instance in Lua
int lua_protobuf_pb_Header_gc(lua_State *L);

// obtain serialized representation of instance
int lua_protobuf_pb_Header_serialized(lua_State *L);

// clear all fields in the message
int lua_protobuf_pb_Header_clear(lua_State *L);

// return size of the message
int lua_protobuf_pb_Header_size(lua_State *L);
// optional int32 packetID = 1
int lua_protobuf_pb_Header_clear_packetID(lua_State *L);
int lua_protobuf_pb_Header_get_packetID(lua_State *L);
int lua_protobuf_pb_Header_set_packetID(lua_State *L);
int lua_protobuf_pb_Header_has_packetID(lua_State *L);

// optional int32 packetSize = 2
int lua_protobuf_pb_Header_clear_packetSize(lua_State *L);
int lua_protobuf_pb_Header_get_packetSize(lua_State *L);
int lua_protobuf_pb_Header_set_packetSize(lua_State *L);
int lua_protobuf_pb_Header_has_packetSize(lua_State *L);

// optional uint32 sourceGuid = 3
int lua_protobuf_pb_Header_clear_sourceGuid(lua_State *L);
int lua_protobuf_pb_Header_get_sourceGuid(lua_State *L);
int lua_protobuf_pb_Header_set_sourceGuid(lua_State *L);
int lua_protobuf_pb_Header_has_sourceGuid(lua_State *L);

// optional uint32 playerID = 4
int lua_protobuf_pb_Header_clear_playerID(lua_State *L);
int lua_protobuf_pb_Header_get_playerID(lua_State *L);
int lua_protobuf_pb_Header_set_playerID(lua_State *L);
int lua_protobuf_pb_Header_has_playerID(lua_State *L);

// optional uint32 gateGuid = 5
int lua_protobuf_pb_Header_clear_gateGuid(lua_State *L);
int lua_protobuf_pb_Header_get_gateGuid(lua_State *L);
int lua_protobuf_pb_Header_set_gateGuid(lua_State *L);
int lua_protobuf_pb_Header_has_gateGuid(lua_State *L);

// optional uint32 playerGuid = 6
int lua_protobuf_pb_Header_clear_playerGuid(lua_State *L);
int lua_protobuf_pb_Header_get_playerGuid(lua_State *L);
int lua_protobuf_pb_Header_set_playerGuid(lua_State *L);
int lua_protobuf_pb_Header_has_playerGuid(lua_State *L);

// repeated uint32 remoteGuids = 7
int lua_protobuf_pb_Header_clear_remoteGuids(lua_State *L);
int lua_protobuf_pb_Header_get_remoteGuids(lua_State *L);
int lua_protobuf_pb_Header_set_remoteGuids(lua_State *L);
int lua_protobuf_pb_Header_size_remoteGuids(lua_State *L);

// repeated uint32 recvPlayerIDs = 8
int lua_protobuf_pb_Header_clear_recvPlayerIDs(lua_State *L);
int lua_protobuf_pb_Header_get_recvPlayerIDs(lua_State *L);
int lua_protobuf_pb_Header_set_recvPlayerIDs(lua_State *L);
int lua_protobuf_pb_Header_size_recvPlayerIDs(lua_State *L);

// optional uint32 ackSequenceNumber = 9
int lua_protobuf_pb_Header_clear_ackSequenceNumber(lua_State *L);
int lua_protobuf_pb_Header_get_ackSequenceNumber(lua_State *L);
int lua_protobuf_pb_Header_set_ackSequenceNumber(lua_State *L);
int lua_protobuf_pb_Header_has_ackSequenceNumber(lua_State *L);

// optional uint32 sequenceNumber = 10
int lua_protobuf_pb_Header_clear_sequenceNumber(lua_State *L);
int lua_protobuf_pb_Header_get_sequenceNumber(lua_State *L);
int lua_protobuf_pb_Header_set_sequenceNumber(lua_State *L);
int lua_protobuf_pb_Header_has_sequenceNumber(lua_State *L);

// end of message Header

#endif