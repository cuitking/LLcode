#ifndef PACKET_H
#define PACKET_H

#include "Types.h"
#include "LuaHelper.h"
#include <string>

class CPlayerState;

namespace pb
{
	class Position;
	class Direction;
}

namespace scgl
{
	//单个逻辑包大小限制为不超过40K
	const int MAX_PACKET_SIZE = 40 * 1024;

	class CLuaHelper;
	class CVariant;
	void ConfigProcessParam(const char* configFile, CLuaHelper::LoadFuntion loader = NULL);
	const CVariant& GetGlobalParam(std::string key);

	int GetPacketID(const char* packetName);
	int GetPacketWay(const char* wayName);
	int GetPacketWay(int packetID);
	const char* GetPacketName(int packetID);
	const char* GetPBName(int packetID);

	void RegisterStateReaction(CPlayerState* state, int stateMask);
	int EncodePacketToBuffer(const scgl::Header& header, const scgl::Message* data, char* buffer, int bufferCapacity);
	bool GetPacketSizeFromBuffer(const char* buffer, int bufferSize, int& packetSize, const char*& headerBuffer, int& headerSize, const char*& dataBuffer, int& dataSize);
	int GetPacketSizeFromPacket(const scgl::Header& header, const scgl::Message* data, int& headerSize, int& dataSize);
	int EncodePacketToBufferEx(const scgl::Header& header, const char* data, int len, char* buffer, int bufferCapacity);
}

#endif

