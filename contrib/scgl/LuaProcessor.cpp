#include "LuaProcessor.h"
#include "Net.h"
#include "LuaHelper.h"
#include "LuaBinder.h"
#include "Packet.h"
#include "../pb/lua-protobuf.h"
#include "../pb/header.pb.h"
#include "../pb/packetSystem.pb.h"
#include "Log.h"
#include "Utility.h"
#include <lua/lua.hpp>
#include <boost/cast.hpp>
#include <boost/thread.hpp>

#include <Windows.h>

namespace scgl
{
	struct CLuaProcessor::Impl
	{
		INet*							net;
		CLuaBinder<CLuaProcessor>		luaBinder;
		int processRef;
		Impl()
			:luaBinder("scglNet")
			, processRef(0)
			, net(NULL)
		{
		}
	};

	int Generate32BitGuid(lua_State* L)
	{
		lua_pushnumber(L, Generate32BitGuid());
		return 1;
	}
	int GenerateUuid(lua_State* L)
	{
		lua_pushstring(L,GenerateUuid());
		return 1;
	}

	CLuaProcessor::CLuaProcessor()
		: m_impl(new Impl)
	{
		GetLuaHelper().PushFunction("luaopen_pb", luaopen_pb);
		GetLuaHelper().PushFunction("Generate32BitGuid", Generate32BitGuid);
		GetLuaHelper().PushFunction("GenerateUuid",GenerateUuid);
		m_impl->luaBinder.BindLua(&GetLuaHelper());
		m_impl->luaBinder.BindObject(this);
		m_impl->luaBinder.Register(&CLuaProcessor::Connect, "Connect");
		m_impl->luaBinder.Register(&CLuaProcessor::CloseConnectionByType, "CloseConnectionByType");
		m_impl->luaBinder.Register(&CLuaProcessor::CloseConnectionByGuid, "CloseConnectionByGuid");
		m_impl->luaBinder.Register(&CLuaProcessor::SendPacket, "Send");
		m_impl->luaBinder.Register(&CLuaProcessor::Broadcast, "Broadcast");
		m_impl->luaBinder.Register(&CLuaProcessor::OpenNAT, "OpenNAT");
		m_impl->luaBinder.Register(&CLuaProcessor::SetGuid, "SetGuid");
		m_impl->luaBinder.Register(&CLuaProcessor::GetGuid, "GetGuid");
		m_impl->luaBinder.Register(&CLuaProcessor::GetClockDifferential, "GetClockDifferential");
		m_impl->luaBinder.Register(&CLuaProcessor::GetWorkService, "GetWorkService");
		m_impl->luaBinder.Register(&CLuaProcessor::ShutDown, "ShutDown");
	}

	CLuaProcessor::~CLuaProcessor(void)
	{
		if (m_impl->processRef)
		{
			lua_State* L = GetLuaHelper().GetLuaState();
			lua_unref(L, m_impl->processRef);
		}
		delete m_impl;
	}

	INet& scgl::CLuaProcessor::GetNet() const
	{
		return *m_impl->net;
	}

	void CLuaProcessor::Process(const scgl::Header& header, const scgl::Message* data)
	{
		lua_State* L = GetLuaHelper().GetLuaState();

		if (!lua_checkstack(L, 6))
		{
			LOG_ERROR("Lua", "Lua stack is not enough，header[" << GetPacketName(header.packetid()) << "].");
			return;
		}

		int dataSize = 0;
		if (data != NULL)
		{
			dataSize = data->ByteSize();
		}

		int n = lua_gettop(L);
		lua_getref(L, m_impl->processRef);
		if (!lua_isfunction(L, -1))
		{
			lua_pop(L, lua_gettop(L) - n);
			return;
		}

		int packetID = header.packetid();
		lua_pushnumber(L, packetID);
		if (header.has_playerid() && header.has_gateguid())
		{
			lua_pushnumber(L, header.playerid());
			lua_pushnumber(L, header.gateguid());
		}

		if (data != NULL)
		{
			//直接传入userdata，由lua gc负责释放
			const char* pbName = scgl::GetPBName(header.packetid());
			::google::protobuf::Message* message = const_cast<scgl::Message*>(data);
			PushMessage(L, message, pbName);
		}

		static const int packetIDUpdate = GetPacketID("PACKET_SYSTEM_ID_UPDATE");
		if (packetIDUpdate != packetID)
		{
			LOG_TRACE("Lua", __FUNCTION__ << " ENTER,[" << GetPacketName(packetID) << "](" << packetID << ")"
				<< "],size[" << dataSize << "]"
				);
		}

		try
		{
			if (lua_pcall(L, lua_gettop(L) - 1 - n, LUA_MULTRET, 0))
			{
				LOG_ERROR("Lua", "脚本执行错误，header[" << GetPacketName(packetID) << "],请检查脚本代码:" << lua_tostring(L, -1));
			}
		}
		catch (...)
		{
			LOG_ERROR("Lua", "Lua system error，header[" << GetPacketName(header.packetid()) << "].");
		}

		lua_pop(L, lua_gettop(L) - n);
	}

	int CLuaProcessor::Connect(lua_State* L)
	{
		if (lua_type(L, 1) == LUA_TSTRING && lua_type(L, 2) == LUA_TNUMBER && lua_type(L, 3) == LUA_TNUMBER && lua_type(L, 4) == LUA_TBOOLEAN)
		{
			unsigned short port = static_cast<unsigned short>(luaL_checknumber(L, 2));
			pb::EConnectionType connectionType = static_cast<pb::EConnectionType>(lua_tointeger(L, 3));
			lua_pushboolean(L, GetNet().Connect(lua_tostring(L, 1), port, connectionType));
			return 1;
		}
		return 0;
	}

	int CLuaProcessor::SendPacket(lua_State* L)
	{
		if (lua_type(L, 1) == LUA_TNUMBER && lua_type(L, 2) == LUA_TLIGHTUSERDATA && lua_type(L, 3) == LUA_TLIGHTUSERDATA)
		{
			scgl::Guid guid = lua_tointeger(L, 1);
			scgl::Header* header = static_cast<scgl::Header*>(lua_touserdata(L, 2));
			if (header->packetid() == 0)
			{
				return luaL_error(L, "SendPacket error: packetID is 0.");
			}
			::google::protobuf::Message* m = static_cast<::google::protobuf::Message*>(lua_touserdata(L, 3));
			lua_pushboolean(L, GetNet().Send(guid, *header, m));
			return 1;
		}

		return 0;
	}

	int CLuaProcessor::Broadcast(lua_State* L)
	{
		luaL_checktype(L, 1, LUA_TNUMBER);
		luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
		luaL_checktype(L, 3, LUA_TLIGHTUSERDATA);

		pb::EConnectionType connectionType = static_cast<pb::EConnectionType>(lua_tointeger(L, 1));
		scgl::Header* header = static_cast<scgl::Header*>(lua_touserdata(L, 2));
		if (header->packetid() == 0)
		{
			return luaL_error(L, "Broadcast error: packetID is 0.");
		}
		::google::protobuf::Message* data = static_cast<::google::protobuf::Message*>(lua_touserdata(L, 3));
		lua_pushboolean(L,  GetNet().Broadcast(connectionType, *header, data));
		return 1;
	}

	int CLuaProcessor::CloseConnectionByType(lua_State* L)
	{
		luaL_checktype(L, 1, LUA_TNUMBER);

		pb::EConnectionType connectionType = static_cast<pb::EConnectionType>(lua_tointeger(L, 1));
		GetNet().CloseByType(connectionType);
		return 0;
	}

	int CLuaProcessor::CloseConnectionByGuid(lua_State* L)
	{
		luaL_checktype(L, 1, LUA_TNUMBER);
		GetNet().CloseByGuid(lua_tointeger(L, 1));
		return 0;
	}

	bool CLuaProcessor::LoadLua(const char* luaPath)
	{
		GetLuaHelper().RunFile(luaPath);
		lua_State* L = GetLuaHelper().GetLuaState();
		int n = lua_gettop(L);
		lua_getglobal(L, "Process");
		if (!lua_isfunction(L, -1))
		{
			lua_pop(L, lua_gettop(L) - n);
			return false;
		}
		m_impl->processRef = lua_ref(L, -1);
		lua_pop(L, lua_gettop(L) - n);
		return true;
	}

	CLuaHelper& CLuaProcessor::GetLuaHelper()
	{
		return SLuaHelper::Instance();
	}

	int CLuaProcessor::OpenNAT(lua_State* L)
	{
		luaL_checktype(L, 1, LUA_TNUMBER);
		luaL_checktype(L, 2, LUA_TSTRING);
		luaL_checktype(L, 3, LUA_TNUMBER);

		Guid guid = static_cast<Guid>(lua_tonumber(L, 1));
		const char* IP = lua_tostring(L, 2);
		unsigned short port = static_cast<unsigned short>(lua_tonumber(L, 3));

		lua_pushboolean(L, GetNet().OpenNAT(guid, IP, port));
		return 1;
	}

	int CLuaProcessor::SetGuid(lua_State* L)
	{
		luaL_checktype(L, 1, LUA_TNUMBER);
		GetNet().SetGuid(static_cast<scgl::Guid>(lua_tonumber(L, 1)));
		return 0;
	}

	int CLuaProcessor::GetGuid(lua_State* L)
	{
		lua_pushnumber(L, GetNet().GetGuid());
		return 1;
	}

	int CLuaProcessor::GetClockDifferential(lua_State* L)
	{
		lua_pushnumber(L, GetNet().GetClockDifferential());
		return 1;
	}

	int CLuaProcessor::GetWorkService( lua_State* L )
	{
		lua_pushlightuserdata(L, GetNet().GetWorkService());
		return 1;
	}

	void CLuaProcessor::SetNet( INet& _net )
	{
		m_impl->net = &_net;
	}

	int CLuaProcessor::ShutDown( lua_State* L )
	{
		GetNet().Shutdown();
		return 0;
	}

}
