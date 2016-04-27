#pragma once

#include "ringbuffer.h"
#include <tbb/atomic.h>

extern long SERVER_SERVER_BUFFER;
extern long MAX_SENDBUFFER;
extern long MAX_RECVBUFFER;
extern long COMMON_MAX_BUFFER;

#include "Types.h"
#include "ConnectionConfig.h"
#include "Win32.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/noncopyable.hpp>
#include <deque>

namespace RakNet
{
	typedef unsigned long long Time;
}

namespace pb
{
	enum EConnectionType;
}


namespace scgl
{

	struct SConnectionConfig;

	enum CloseStatus
	{ 
		CLOSESTATUS_OPEN = 0,		//正常状态，双向链路通信 
		CLOSESTATUS_1_CLOSING,		//本端主动执行关闭
		CLOSESTATUS_2_CLOSING,		//本端被动执行关闭
		CLOSESTATUS_CLOSE_WRITE,	//关闭了写链路
		CLOSESTATUS_CLOSE_FULL		//对方也关闭了写链路
	};

	// 此类中所有的项都可能被多线程访问，请做好相关工作
	class CConnection : private boost::noncopyable
	{
	public:
		CConnection(const SConnectionConfig& _connectionConfig, boost::asio::io_service* io_service = NULL, bool _isClientNode = false);
		~CConnection();

		void CreateBuffer(size_t readBufferCapacity);
	private:
		void Clear();

	public:
		const SConnectionConfig		config;				// 必须放在最前边保证第一个被初始化
		const pb::EConnectionType	connectionType;		//连接类型 保持和config里的type一致
		boost::asio::ip::tcp::socket* socket;
		CloseStatus				closeStatus;	//当前关闭状态
		tbb::atomic<bool>		writting;		//当前是否投递了异步发送
		RingBuffer*				readBuffer;		//接收数据缓冲区
		scgl::Guid				guid;
		bool					needReconnect;
		bool					isClientNode;
		time_t					loseTime;
		unsigned int			sendSequenceNumber; //当前发送包序号
		unsigned int			ackSequenceNumber; //自己收到对方的数据包序号
		unsigned int			peer_ackSequenceNumber; //对方接收到的确认包序号
		SendDataDeque			sendQueue;				//当前发送队列	
		std::deque<SSendBufInfo>	waitAckQueue;		//等待确认的数据包
		time_t					lastAliveTime;			//上次存活时间
		bool					inited;					//链路是否已经初始化成功
		int						retryCount;				//尝试重连次数
	};

	const char* GetConnectionTypeName(pb::EConnectionType type);
	pb::EConnectionType GetConnectionType(const char* name);
}
