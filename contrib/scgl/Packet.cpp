#include "Packet.h"
#include "Log.h"
#include "Variant.h"
#include "LuaHelper.h"
#include "Utility.h"
#pragma warning( push )
#pragma warning( disable : 4244)
#include <pb/header.pb.h>
#pragma warning( pop )

namespace scgl
{

	int GetPacketSizeFromPacket( const Header& header, const Message* data, int& headerSize, int& dataSize )
	{
		headerSize = header.ByteSize();
		dataSize = 0;
		if (data != NULL)
		{
			dataSize = data->ByteSize();
		}
		return sizeof(int)+sizeof(int)+headerSize+dataSize;
	}

	int EncodePacketToBuffer( const scgl::Header& header, const scgl::Message* data, char* buffer, int bufferCapacity )
	{
		int headerSize;
		int dataSize;
		int packetSize = GetPacketSizeFromPacket(header, data, headerSize, dataSize);
		if (packetSize > bufferCapacity)
		{
			LOG_ERROR("Net", "Can't do pack(" << GetPacketName(header.packetid()) << "),packetSize(" << packetSize << ") is larger than bufferCapacity(" << bufferCapacity << ").");
			return 0;
		}

		*(int*)(buffer) = headerSize;
		*(int*)(buffer+sizeof(int)) = dataSize;

		if (!header.SerializeToArray(buffer+sizeof(int)+sizeof(int), headerSize))
		{
			LOG_ERROR("Net", "Serialize header error,packet(" << GetPacketName(header.packetid()) << "), packetSize(" << packetSize << "),bufferCapacity(" << bufferCapacity << ").");
			return 0;
		}

		if (data != NULL)
		{
			if (!data->SerializeToArray(buffer+sizeof(int)+sizeof(int)+headerSize, dataSize))
			{
				LOG_ERROR("Net", "Serialize data error,packet(" << GetPacketName(header.packetid()) << "), packetSize(" << packetSize << "),bufferCapacity(" << bufferCapacity << ").");
				return 0;
			}
		}

		return packetSize;
	}

	bool GetPacketSizeFromBuffer(const char* buffer, int bufferSize, int& packetSize, const char*& headerBuffer, int& headerSize, const char*& dataBuffer, int& dataSize)
	{
		(void)bufferSize;
		headerSize = *(int*)(buffer);
		dataSize = *(int*)(buffer+sizeof(int));
		packetSize = sizeof(int)+sizeof(int)+headerSize+dataSize;
		if (bufferSize == packetSize)
		{
			headerBuffer = buffer+sizeof(int)+sizeof(int);
			dataBuffer = headerBuffer+headerSize;
			return true;
		}
		return false;
	}

	int EncodePacketToBufferEx( const scgl::Header& header, const char* data, int len, char* buffer, int bufferCapacity )
	{
		int headerSize = header.ByteSize();
		int packetSize = sizeof(int)+sizeof(int)+headerSize+len;
		if (packetSize > bufferCapacity)
		{
			LOG_ERROR("Net", "Can't do pack(" << GetPacketName(header.packetid()) << "),packetSize(" << packetSize << ") is larger than bufferCapacity(" << bufferCapacity << ").");
			return 0;
		}

		*(int*)(buffer) = headerSize;
		*(int*)(buffer+sizeof(int)) = len;

		if (!header.SerializeToArray(buffer+sizeof(int)+sizeof(int), headerSize))
		{
			LOG_ERROR("Net", "Serialize header error,packet(" << GetPacketName(header.packetid()) << "), packetSize(" << packetSize << "),bufferCapacity(" << bufferCapacity << ").");
			return 0;
		}

		if (data != NULL && len > 0)
		{
			memcpy(buffer+sizeof(int)+sizeof(int)+headerSize, data, len);
		}

		return packetSize;
	}
}
