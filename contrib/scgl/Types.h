#ifndef TYPES_H
#define TYPES_H

#include "container_fwd.h"
#include <boost_fwd.h>
#include <tbb/concurrent_queue.h>

typedef struct lua_State lua_State;

namespace pb
{
	class Header;
	enum EConnectionType;
}

namespace google
{
	namespace protobuf
	{
		class Message;
		class MessageLite;
	}
}

namespace tbb
{
	template<typename T>
	struct atomic;
}

namespace gp = google::protobuf;

namespace scgl
{
	typedef unsigned int PlayerIDType;
	typedef unsigned int	Guid;

	typedef struct _SSendBufInfo
	{
		char* data;
		int len;
		unsigned int sequenceNumber;
	}SSendBufInfo;

	typedef tbb::concurrent_queue<SSendBufInfo>	SendDataDeque;

	enum { INVALID_PLAYERID = 0 };

	enum { INVALID_GUID = 0 };

	enum { MAX_STRING_LENGTH = 2048 };

	class INet;
	class IPacketProcessor;
	struct SConnectionConfig;

	typedef std::vector<char>	Buffer;

	class CBuffer;
	class CConnection;
	typedef ::boost::shared_ptr<CConnection> ConnectionPtr;

	class IConnectionManager;
	class CPropertyMap;

	typedef ::pb::Header				Header;
	typedef ::google::protobuf::Message	Message;

	typedef ::boost::shared_ptr<Header>		HeaderPtr;
	typedef ::boost::shared_ptr<Message>	MessagePtr;

	typedef boost::asio::high_resolution_timer						Timer;
	typedef boost::shared_ptr<Timer>						TimerPtr;

	typedef boost::function<void(ConnectionPtr, Header&, Message*)>	Processor;

	enum EncodeStatus
	{ 
		ENCODESTATUS_SUCCESS, 
		ENCODESTATUS_ERROR,
		ENCODESTATUS_NORMAL 
	};

	typedef boost::function<EncodeStatus(ConnectionPtr, const char*, size_t, char*, size_t&)>	EncodeProcess;

	enum DecodeStatus
	{ 
		DECODESTATUS_SUCCESS, 
		DECODESTATUS_ERROR,
		DECODESTATUS_NORMAL 
	};
	typedef boost::function<DecodeStatus(ConnectionPtr, const char*, size_t, char*, size_t&)>	DecodeProcess;
	typedef boost::function<bool(ConnectionPtr, Header*, const char*, int)>	PrepareProcess;
	typedef boost::function<bool(ConnectionPtr)>			InitConnectionProcess;
	typedef boost::function<void(const char*, ConnectionPtr, const Header&, const Message*)>			DisplayMessageProcess;

	typedef boost::shared_ptr<EncodeProcess>				EncodeProcessPtr;
	typedef boost::shared_ptr<DecodeProcess>				DecodeProcessPtr;
	typedef boost::shared_ptr<PrepareProcess>				PrepareProcessPtr;
	typedef boost::shared_ptr<InitConnectionProcess>				InitConnectionProcessPtr;
	typedef boost::shared_ptr<DisplayMessageProcess>				DisplayMessageProcessPtr;
	typedef boost::function<void()>	Functor;
}

#endif