#include "PacketDetail.h"
#include "LuaHelper.h"
#include "Table.h"
#include "Variant.h"
#include "Packet.h"
#include "PlayerState.h"
#include "Log.h"
#include <string>

namespace scgl
{
	//全局配置表
	PacketDetailMap packetDetailMap;
	scgl::CTable global_config;

	const SPacketDetail& PushPacketDetail( const CVariant &value, PacketDetailMap& packetDetailMap)
	{
		if (value.GetMetaType() == CVariant::TYPE_TABLE)
		{
			const CTable& packet = value.GetTable();
			int packetID = packet["ID"].GetNumber<int>();
			const char* packetName = packet["name"].GetString();
			int packetWay = packet["way"].GetNumber<int>();
			int packetFilter = packet["filter"].GetNumber<int>();
			const char* pbName = packet["pbName"].GetString();
			LOG_TRACE("Packet", __FUNCTION__ << ":" << packetID << "," << packetName << "," << packetWay << "," << pbName);
			return *(packetDetailMap.insert(SPacketDetail(packetID, packetWay, packetName, packetFilter, pbName)).first);
		}
		throw std::logic_error("error packet detail from GetPacketDetail");
	}

	void ConfigProcessParam( const char* configFile, CLuaHelper::LoadFuntion loader /*= NULL*/ )
	{
		if (loader != NULL)
		{
			SLuaHelper::Instance().SetLoadFunction(loader);
		}

		SLuaHelper::Instance().PushString("JSON_CONFIG", configFile);
		SLuaHelper::Instance().RunString("package.path = './?.lua;./script/?.lua;../script/?.lua;./script/declare/?.lua;../script/declare/?.lua'");
		SLuaHelper::Instance().RunString("package.cpath = './?.dll;./script/?.dll;../script/?.dll'");
		SLuaHelper::Instance().RunString("require('log')");
		SLuaHelper::Instance().RunString("require('packet')");

		//加载启动配置文件
		{
			VariantVector parameters;
			VariantVector results;
			SLuaHelper::Instance().CallFunction("GetConfigParam", &parameters, &results);
			if (!results.empty() && results[0].GetMetaType() == CVariant::TYPE_TABLE)
			{
				global_config = results[0].GetTable();
			}
		}

		//加载包定义
		{
			VariantVector parameters;
			VariantVector results;
			SLuaHelper::Instance().CallFunction("GetPacketDetail", &parameters, &results);
			if (!results.empty() && results[0].GetMetaType() == CVariant::TYPE_TABLE)
			{
				const CTable::VariantMap& packets = results[0].GetTable().GetAllValue();
				for (CTable::VariantMap::const_iterator it = packets.begin(); it != packets.end(); ++it)
				{
					const CVariant& value = it->second;
					PushPacketDetail(value, packetDetailMap);
				}
			}
		}
	}

	const CVariant& GetGlobalParam(std::string key)
	{
		return global_config[key];
	}

	const char* GetPacketName(int packetID)
	{
		const PacketDetailMap::iterator& iter = packetDetailMap.get<SPacketID>().find(packetID);
		if (iter != packetDetailMap.end())
		{
			const SPacketDetail& packet = *iter;
			return packet.packetName.c_str();
		}
		LOG_ERROR("Packet", __FUNCTION__ << ",(" << packetID << ")'s packetName not exist!");
		return "";
	}

	const char* GetPBName(int packetID)
	{
		const PacketDetailMap::iterator& iter = packetDetailMap.get<SPacketID>().find(packetID);
		if (iter != packetDetailMap.end())
		{
			const SPacketDetail& packet = *iter;
			return packet.pbName.c_str();
		}
		LOG_ERROR("Packet", __FUNCTION__ << ",(" << packetID << ")'s pbName not exist!");
		return "";
	}

	int GetPacketID(const char* packetName)
	{
		const PacketDetailMap::index<SPacketName>::type& packetMap = packetDetailMap.get<SPacketName>();
		PacketDetailMap::index<SPacketName>::type::iterator iter = packetMap.find(packetName);
		if (iter != packetMap.end())
		{
			const SPacketDetail& packet = *iter;
			return packet.packetID;
		}
		LOG_ERROR("Packet", __FUNCTION__ << ",(" << packetName << ")'s packetID not exist!");
		return 0;
	}

	int GetPacketWay(const char* wayName)
	{
		VariantVector parameters;
		parameters.push_back(wayName);
		VariantVector results;
		SLuaHelper::Instance().CallFunction("GetPacketWay", &parameters, &results);
		if (!results.empty() && results[0].IsNumber())
		{
			return results[0].GetNumber<int>();
		}
		LOG_ERROR("Packet", __FUNCTION__ << ",(" << wayName << ") not exist!");
		return 0;
	}

	int GetPacketWay(int packetID)
	{
		const PacketDetailMap::iterator& iter = packetDetailMap.get<SPacketID>().find(packetID);
		if (iter != packetDetailMap.end())
		{
			const SPacketDetail& packet = *iter;
			return packet.packetWay;
		}
		LOG_ERROR("Packet", __FUNCTION__ << ",(" << packetID << ")'s packetWay not exist!");
		return 0;
	}

	void RegisterStateReaction(CPlayerState* state, int stateMask)
	{
		for (PacketDetailMap::iterator it = packetDetailMap.begin(); it != packetDetailMap.end(); ++it)
		{
			const SPacketDetail& packet = *it;
			if (packet.packetFilter & stateMask)
			{
				state->RegisterReaction(state->GetState(), packet.packetID, state->GetState());
			}
		}
	}
}
