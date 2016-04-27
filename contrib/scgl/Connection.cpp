#include "Connection.h"
#include "StringAlgorithm.h"
#pragma warning( push )
#pragma warning( disable : 4244)
#include <pb/connectionType.pb.h>
#pragma warning( pop )

#include <boost/algorithm/string.hpp>

//默认每个玩家连接的最大发送缓冲区(作废)
long MAX_SENDBUFFER = 1<<18;

//默认每个玩家连接的最大接收缓冲区
long MAX_RECVBUFFER = 1<<17;

//默认服务器之间连接的缓冲区
long SERVER_SERVER_BUFFER = 1<<21;

#define my_malloc	malloc
#define my_free		free

namespace scgl
{
	const int DEFAULT_BUFFER_CAPACITY = 8 * 1024;

	CConnection::CConnection(const SConnectionConfig& _config, boost::asio::io_service* io_service, bool _isClientNode)
		: config(_config)
		, connectionType(config.type)
		, socket(NULL)
		, guid(INVALID_GUID)
		, readBuffer(NULL)
		, closeStatus(CLOSESTATUS_OPEN)
		, isClientNode(_isClientNode)
	{
		writting = false;
		needReconnect = true;
		inited = false;
		loseTime = 0;
		sendSequenceNumber = 0;
		ackSequenceNumber = 0;
		peer_ackSequenceNumber = 0;
		lastAliveTime = 0;
		retryCount = 0;
		// udp连接不需要io_service和socket
		if (config.tcp && io_service != NULL)
		{
			socket = new boost::asio::ip::tcp::socket(*io_service); 
		}
	}

	CConnection::~CConnection()
	{	
		Clear();
	}

	void CConnection::CreateBuffer( size_t readBufferCapacity )
	{
		RingBuffer* readBuf = (RingBuffer*)my_malloc(sizeof(RingBuffer));
		char* readData = (char*)my_malloc(readBufferCapacity);
		RingBuffer_Init(readBuf, readBufferCapacity, readData);
		readBuffer = readBuf;
	}

	void CConnection::Clear()
	{
		while(!waitAckQueue.empty())
		{
			//释放待确认数据包
			const SSendBufInfo& buf = waitAckQueue.front();
			delete[] buf.data;
			waitAckQueue.pop_front();
		}
		if (readBuffer != NULL)
		{
			my_free(readBuffer->buffer);
		}
		my_free(readBuffer);
		readBuffer = NULL;
		if (socket != NULL)
		{
			delete socket;
			socket = NULL;
		}
		SSendBufInfo tmpBuf;
		while (sendQueue.try_pop(tmpBuf))
		{
			delete[] tmpBuf.data;
		}
	}

	pb::EConnectionType GetConnectionType( const char* name )
	{
		std::string typeName(name);
		boost::trim(typeName);
		boost::to_lower(typeName);

		if (typeName == "manager" || typeName == "managerserver")
		{
			return pb::CONNECTION_TYPE_MANAGER;
		}
		else if (typeName == "gate" || typeName == "gateserver")
		{
			return pb::CONNECTION_TYPE_GATE;
		}
		else if (typeName == "pve" || typeName == "pveserver" || typeName == "pvegameserver")
		{
			return pb::CONNECTION_TYPE_PVE;
		}
		else if (typeName == "dispatch" || typeName == "dispatcher" || typeName == "dispatchserver" || typeName == "gatedispatch")
		{
			return pb::CONNECTION_TYPE_DISPATCHER;
		}
		else if (typeName == "exchange")
		{
			return pb::CONNECTION_TYPE_EXCHANGE;
		}
		else if (typeName == "relation" || typeName == "relationserver")
		{
			return pb::CONNECTION_TYPE_RELATION;
		}
		else if (typeName == "proxy" || typeName == "udpproxy")
		{
			return pb::CONNECTION_TYPE_PROXY;
		}
		else if (typeName == "player")
		{
			return pb::CONNECTION_TYPE_PLAYER;
		}
		else if (typeName == "gm")
		{
			return pb::CONNECTION_TYPE_GM;
		}else if (typeName == "crossserver")
		{
			return pb::CONNECTION_TYPE_CROSS;
		}

		return pb::CONNECTION_TYPE_UNKNOWN;
	}

	const char* GetConnectionTypeName( pb::EConnectionType type )
	{
		switch (type)
		{
		case pb::CONNECTION_TYPE_SELF:
			return "Self";
		case pb::CONNECTION_TYPE_GATE:
			return "Gate";
		case pb::CONNECTION_TYPE_PVE:
			return "PVE";
		case pb::CONNECTION_TYPE_DISPATCHER:
			return "Dispatcher";
		case pb::CONNECTION_TYPE_MANAGER:
			return "Manager";
		case pb::CONNECTION_TYPE_EXCHANGE:
			return "Exchange";
		case pb::CONNECTION_TYPE_RELATION:
			return "Relation";
		case pb::CONNECTION_TYPE_PROXY:
			return "Proxy";
		case pb::CONNECTION_TYPE_PLAYER:
			return "Player";
		case pb::CONNECTION_TYPE_GM:
			return "GM";
		case pb::CONNECTION_TYPE_CROSS:
			return "CrossServer";
		default:
			return "Unknown";
		}
	}

}
