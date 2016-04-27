#pragma once

#include "Types.h"

namespace scgl
{
	class CLuaHelper;
	class CLuaProcessor
	{
	public:
		explicit CLuaProcessor();
		~CLuaProcessor(void);

	public:
		void Process(const scgl::Header& header, const scgl::Message* data);
		bool LoadLua(const char* luaPath);
		CLuaHelper& GetLuaHelper();
		void SetNet(INet& _net);

	private:
		INet& GetNet() const;
		int Connect(lua_State* L);
		int SendPacket(lua_State* L);
		int Broadcast(lua_State* L);
		int OpenNAT(lua_State* L);
		int SetGuid(lua_State* L);
		int GetGuid(lua_State* L);
		int CloseConnectionByType(lua_State* L);
		int CloseConnectionByGuid(lua_State* L);
		int GetClockDifferential(lua_State* L);
		int GetWorkService(lua_State* L);
		int ShutDown(lua_State* L);

	private:
		struct Impl;
		Impl* m_impl;
	};
}
