#pragma once

#include "Types.h"
#include <sstream>
#include <iomanip>
#include <ios>

namespace scgl
{

	void DumpData(const char* data, int len, std::ostringstream& oss);
	class INet
	{
	public:
		enum PacketPriority
		{
			PACKET_PRIORITY_DEFAULT		= 0,
			PACKET_PRIORITY_DB			= 1000,
			PACKET_PRIORITY_UPDATE		= 2000,
			PACKET_PRIORITY_DEBUG		= 3000,
		};

		INet(void);
		virtual ~INet(void);

	public:
		virtual bool Broadcast(pb::EConnectionType type, scgl::Header& header, const scgl::Message* data = 0, bool disableXP = false)  = 0;
		virtual bool Send(scgl::Guid guid, scgl::Header& header, const scgl::Message* data = 0, bool disableXP = false)  = 0;
		virtual void CloseByGuid(scgl::Guid guid) = 0;
		virtual void CloseByType(pb::EConnectionType type) = 0;
		virtual scgl::Guid GetGuid() const = 0;
		virtual void SetGuid(scgl::Guid newGuid) = 0;
		virtual bool IsType(scgl::ConnectionPtr connection, pb::EConnectionType type) const = 0;
		virtual	bool Connect(const char* remoteIP, unsigned short remotePort, pb::EConnectionType type) = 0;
		virtual bool OpenNAT(scgl::Guid guid, const char* IP, unsigned short port) = 0;
		virtual pb::EConnectionType GetMyServiceType() const = 0;
		virtual pb::EConnectionType GetTypeByGuid(scgl::Guid guid) const = 0;
		virtual const char* GetListenIPByType(pb::EConnectionType type) const = 0;
		virtual unsigned short GetListenPortByType(pb::EConnectionType type) const = 0;
		virtual bool GetRemoteIP(scgl::Guid guid, char* ip, int length) const = 0;
		virtual void ChangeRemoteGuid(scgl::Guid guid, scgl::Guid newGuid) = 0;
		virtual void SendInitServer(scgl::Guid serverGuid) = 0;
		virtual void Shutdown() = 0;
		virtual int Run() = 0;
		virtual int NonblockingRun() = 0;
		virtual void SetClockDifferential(int clockDiff) = 0;
		virtual int GetClockDifferential() = 0;
		virtual void SetDisplayMessageProcess(DisplayMessageProcessPtr displayMessage) = 0;
		virtual void SetEnDecodeProcess(EncodeProcessPtr encode, DecodeProcessPtr decode, PrepareProcessPtr prepare, InitConnectionProcessPtr initConnection) = 0;
		virtual void PushProcessor(Processor processor) = 0;
		virtual void RegisterTimedFunctor(int milliseconds, Functor functor, bool repeat = false, bool iothread = false) = 0;
		virtual void PostPacket(ConnectionPtr source, scgl::Header* header, scgl::Message* data = 0, int priority = PACKET_PRIORITY_DEFAULT) = 0;
		virtual void EnablePacketTimeStat() = 0;
		virtual void DisablePacketTimeStat() = 0;
		virtual void LogPacketStat() const = 0;
		virtual void EnablePacketStat() = 0;
		virtual void DisablePacketStat() = 0;
		virtual int DecodeRingBuffer(ConnectionPtr connection) = 0;
		virtual int DecodeBuffer(ConnectionPtr remoteGuid, const char* buffer, int bufferCapacity) = 0;
		virtual void StartUp(bool isServer, bool needPostUpdate = true, int writeInteval = 30) = 0;
		virtual void SetNoDelay(scgl::Guid guid, bool nodelay) = 0;
		virtual bool RawSend(scgl::Guid guid, scgl::Header& header, const char*, int len, bool disableXP = false) = 0;
		virtual boost::asio::io_service* GetWorkService() = 0;
		virtual bool InitConnection(ConnectionPtr connection) = 0;
		virtual void Post(Functor f, bool iothread = false) = 0;
	};

}
