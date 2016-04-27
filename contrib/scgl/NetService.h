#ifndef SCGL_NETSERVICE_H
#define SCGL_NETSERVICE_H
#pragma once

#include "Types.h"
#include "Net.h"
#include "ConnectionManager.h"
#include "LuaHelper.h"

namespace pb
{
	class Header;
}

namespace scgl
{
	class CTcpHandler;
}

namespace scgl
{
	class CLuaProcessor;
	class CConnection;
	struct SConnectionConfig;

	class CNetService : public INet, public IConnectionManager
	{

	public:
		explicit CNetService(const char* serviceName, const char* configFile = 0, unsigned int updateInteval = 100);
		~CNetService(void);
		bool IsActive() const;
		virtual const char* GetListenIPByType(pb::EConnectionType type) const;
		virtual unsigned short GetListenPortByType(pb::EConnectionType type) const;
		bool GetRemoteIP(scgl::Guid guid, char* ip, int length) const;
		void SendInitServer(scgl::Guid serverGuid);
		scgl::Guid GetGuid() const;
		void PostPacket(ConnectionPtr connection, scgl::Header* header, scgl::Message* data, int priority = 0);
		int DecodeRingBuffer(ConnectionPtr connection);
		int DecodeBuffer(ConnectionPtr connection, const char* buffer, int bufferCapacity);
		void SetDisplayMessageProcess(DisplayMessageProcessPtr displayMessage);
		void SetEnDecodeProcess(EncodeProcessPtr encode, DecodeProcessPtr decode, PrepareProcessPtr prepare, InitConnectionProcessPtr initConnection);
		void StartUp(bool isServer, bool needPostUpdate, int writeInteval);
		void SetNoDelay(scgl::Guid guid, bool nodelay);
		bool RawSend(scgl::Guid guid, scgl::Header& header, const char* data, int len, bool disableXP);
		boost::asio::io_service* GetWorkService();
		bool InitConnection(ConnectionPtr connection);
		void Post(Functor f, bool iothread = false);

	public:
		void ChangeRemoteGuid(scgl::Guid guid, scgl::Guid newGuid);
		bool Connect(const char* remoteIP, unsigned short remotePort, pb::EConnectionType type);
		bool Broadcast(pb::EConnectionType type, scgl::Header& header, const scgl::Message* data, bool disableXP);
		bool Send(scgl::Guid guid, scgl::Header& header, const scgl::Message* data, bool disableXP);
		void CloseByType(pb::EConnectionType type);
		void CloseByGuid(scgl::Guid guid);
		void Shutdown();
		void ShutDownCore();
		bool OpenNAT(scgl::Guid guid, const char* IP, unsigned short port);
		void PushProcessor(Processor processor);
		void RegisterTimedFunctor(int milliseconds, Functor functor, bool repeat = false, bool iothread = false);
		int Run();
		int NonblockingRun();
		void SetNeedDelPacket();
		void LogPacketStat() const;

	private:
		pb::EConnectionType GetMyServiceType() const;
		pb::EConnectionType GetTypeByGuid(scgl::Guid guid) const;

	private:
		const ConnectionPtr FindConnection(scgl::Guid guid) const;
		int InsertConnection(ConnectionPtr connection);
		void SetGuid(scgl::Guid newGuid);
		bool IsType(scgl::ConnectionPtr connection, pb::EConnectionType type) const;
		bool Write( const ConnectionPtr connection, char * data, int size, bool disableXP);
		void Close( const ConnectionPtr connection);

	private:
		bool DecodePacket(ConnectionPtr connection, int packetSize, const char* headerBuffer, int headerSize, const char* dataBuffer, int dataSize);
		bool Listen();
		void SetClockDifferential(int clockDiff);
		int GetClockDifferential();
		scgl::Message* DecodeMessage(int packetID, const void* data, int size);
		void DecodeData(ConnectionPtr connection, const char* dataBuf, int dataLen);
		void ActiveUpdateTimer(TimerPtr timer);
		void OnTimer(TimerPtr timer, Functor functor, bool repeat, int milliseconds);
		void ProcessAllPacket();
		void ProcessPacket(ConnectionPtr source, HeaderPtr headerPtr, scgl::Message* data);
		void EnablePacketTimeStat();
		void DisablePacketTimeStat();
		void EnablePacketStat();
		void DisablePacketStat();

	private:
		void Heartbeat();
		void ProcessAck(ConnectionPtr source, scgl::Header* header);

	private:
		struct Impl;
		Impl* m_impl;
	};

}
#endif
