#include "NetService.h"
#include "Types.h"
#include "Packet.h"
#include "ConnectionConfig.h"
#include "LuaHelper.h"
#include "Variant.h"
#include "Log.h"
#include "Utility.h"
#include "capi.h"
#include "TcpHandler.h"
#include "Connection.h"
#include "StringAlgorithm.h"
#include <lua/lua.hpp>

#pragma warning( push )
#pragma warning( disable : 4244)
#include <pb/lua-protobuf.h>
#include "../pb/header.pb.h"
#include "../pb/packetSystem.pb.h"
#include <google/protobuf/descriptor.h>

#pragma warning( pop )
#pragma warning( push )
#pragma warning( disable : 4702)
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#pragma warning( pop )

#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_priority_queue.h>
#include <tbb/atomic.h>
#include <boost/cast.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/chrono.hpp>
#include <vector>
#include <map>
#include <deque>
#include <sstream>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>

namespace gp = google::protobuf;
namespace bc = boost::chrono;

namespace scgl
{

	//逻辑和IO线程直接交互的数据结构
	typedef struct SProcessData
	{
		ConnectionPtr source;
		HeaderPtr headerPtr;
		scgl::Message* data;
		int				priority;

		SProcessData()
			: data(NULL)
			, priority(0)
		{
		}

		SProcessData(ConnectionPtr _source, HeaderPtr _headerPtr, scgl::Message* _data, int _priority)
			: source(_source)
			, headerPtr(_headerPtr)
			, data(_data)
			, priority(_priority)
		{
		}

		bool operator<(const SProcessData& rhs) const
		{
			return priority < rhs.priority;
		}
	}SProcessData;

	struct SConnectionGuid {};
	struct SConnectionType {};
	namespace bm = boost::multi_index;
	typedef boost::multi_index_container<
		ConnectionPtr,
		bm::indexed_by<
		bm::hashed_unique<
		bm::tag<SConnectionGuid>,
		bm::member<CConnection, scgl::Guid, &CConnection::guid>
		>,
		bm::ordered_non_unique<
		bm::tag<SConnectionType>,
		bm::member<CConnection, const pb::EConnectionType, &CConnection::connectionType>
		>
		>
	> ConnectionDetailMap;

	typedef tbb::concurrent_queue<SProcessData>	ProcessDataDeque;
	typedef std::vector<Processor>	ProcessorVector;

	typedef struct SPacketStatistics
	{
		int							packetID;
		unsigned long long			amount;
		bc::nanoseconds				time;
		unsigned long long			sentSize;
		SPacketStatistics(int _packetID, unsigned long long _sentSize = 0)
			: packetID(_packetID)
			, amount(1)
			, time(0)
			, sentSize(_sentSize)
		{

		}
	}SPacketStatistics;
	typedef std::map<int, SPacketStatistics>	IntPacketStatisticsMap;
	const int MAX_PACKET_LIST_CAPACITY = 2048;

	struct CNetService::Impl
	{
		ConnectionConfigVector			connectionConfigs;
		boost::asio::io_service			io_service;
		boost::recursive_mutex			connections_mutex;
		ConnectionDetailMap				connections;
		ProcessorVector					processorList;
		std::vector<SConnectionConfig*> managerConfigs;
		Guid							myID;
		bool							isActive;
		unsigned int					updateInteval;
		int								clockDifferential;
		bool							needDelPacket;
		bool							needPostUpdate;
		ProcessDataDeque				debugPacketList;
		ProcessDataDeque				updatePacketList;
		ProcessDataDeque				dbPacketList;
		ProcessDataDeque				defaultPacketList;
		CTcpHandler						tcpHandler;

		//自定义打包/解包接口
		boost::shared_ptr<EncodeProcess> encodeFunc;
		boost::shared_ptr<DecodeProcess> decodeFunc;
		boost::shared_ptr<PrepareProcess> prepareFunc;
		boost::shared_ptr<InitConnectionProcess> initConnectionFunc;
		boost::shared_ptr<DisplayMessageProcess> displayMessageFunc;

		//包统计相关
		IntPacketStatisticsMap				solvedStatistics;
		bool								needStatTime;
		IntPacketStatisticsMap				sentStatistics;
		bool								needStat;
		bool							isServerNet;
		pb::EConnectionType				serviceType;

		//上次Update的时间点
		bc::high_resolution_clock::time_point lastUpdateTimePoint;
#ifndef _WIN32
		tbb::atomic<bool>		isPosted;		//当前是否投递了异步通知
#endif
		Impl(const char* serviceName, INet& net, IConnectionManager& connectionManager)
			: myID(INVALID_GUID)
			, isActive(false)
			, tcpHandler(net, connectionManager)
			, clockDifferential(0)
			, needDelPacket(false)
			, needPostUpdate(false)
#if defined(_DEBUG) || defined(DEBUG)
			, needStatTime(true)
			, needStat(true)
#else
			, needStatTime(false)
			, needStat(false)
#endif
		{
			serviceType = scgl::GetConnectionType(serviceName);
#ifndef _WIN32
			isPosted = false;
#endif
		}

		~Impl()
		{
		}
	};


	CNetService::CNetService(const char* serviceName, const char* configFile /*= NULL*/, unsigned int updateInteval /*= 100*/)
		: m_impl(NULL)
	{
		m_impl = new Impl(serviceName, *this, *this);
		m_impl->updateInteval = updateInteval;

		const scgl::CVariant& guidValue = scgl::GetGlobalParam("guid");
		if (guidValue != scgl::CVariant::NIL)
		{
			m_impl->myID = guidValue.GetNumber<int>();
		}
		else
		{
			m_impl->myID = scgl::Generate32BitGuid();
		}

		if (configFile != NULL)
		{
			//服务器端初始化
			boost::property_tree::ptree ptree;
			boost::filesystem::ifstream f(configFile);
			boost::property_tree::read_xml(f, ptree);
			ptree = ptree.get_child("Settings");

			boost::property_tree::ptree::const_assoc_iterator iter = ptree.find("Servers");
			if (ptree.to_iterator(iter) != ptree.end())
			{
				BOOST_FOREACH(const boost::property_tree::ptree::value_type & v, ptree.get_child("Servers"))
				{
					const boost::property_tree::ptree& p = v.second;
					unsigned short maxConnectionAmount = p.get<unsigned short>("MaxConnectionAmount");
					std::string listenIP = p.get<std::string>("ListenIP");
					unsigned short listenPort = p.get<unsigned short>("ListenPort");
					std::string typeName = p.get<std::string>("Type");
					bool udp = (p.get<std::string>("Udp", "False") == "True");
					unsigned int recvBuff = p.get<unsigned int>("RecvBuffer", 0);
					unsigned int sendBuff = p.get<unsigned int>("SendBuffer", 0);
					if (typeName == "Player")
					{
						//获取玩家连接的ringbuffer配置
						MAX_SENDBUFFER = p.get<unsigned int>("WriteRingBufferCapacity", 0);
						MAX_RECVBUFFER = p.get<unsigned int>("ReadRingBufferCapacity", 0);
					}
					SConnectionConfig config(GetConnectionType(typeName.c_str()));
					config.tcp = !udp;
					config.listener = true;
					config.maxConnectionAmount = maxConnectionAmount;
					scgl::CopyStringSafely(config.IP, listenIP.c_str());
					config.port = listenPort;
					config.readBufferCapacity = recvBuff;
					config.writeBufferCapacity = sendBuff;
					m_impl->connectionConfigs.push_back(config);
				}
			}

			iter = ptree.find("Clients");
			if (ptree.to_iterator(iter) != ptree.end())
			{
				BOOST_FOREACH(const boost::property_tree::ptree::value_type & v, ptree.get_child("Clients"))
				{
					const boost::property_tree::ptree& p = v.second;
					std::string typeName = p.get<std::string>("Type");
					bool udp = (p.get<std::string>("Udp", "False") == "True");
					unsigned int recvBuff = p.get<unsigned int>("RecvBuffer", 0);
					unsigned int sendBuff = p.get<unsigned int>("SendBuffer", 0);
					SConnectionConfig config(GetConnectionType(typeName.c_str()));
					config.tcp = !udp;
					config.listener = false;
					config.maxConnectionAmount = 1;
					config.readBufferCapacity = recvBuff;
					config.writeBufferCapacity = sendBuff;
					m_impl->connectionConfigs.push_back(config);
				}
			}

			iter = ptree.find("ManagerServers");
			if (ptree.to_iterator(iter) != ptree.end())
			{
				BOOST_FOREACH(const boost::property_tree::ptree::value_type & v, ptree.get_child("ManagerServers"))
				{
					const boost::property_tree::ptree& p = v.second;
					SConnectionConfig* config = new SConnectionConfig(pb::CONNECTION_TYPE_MANAGER);
					scgl::CopyStringSafely(config->IP, p.get<std::string>("ManagerIP").c_str());
					config->port = p.get<unsigned short>("ManagerPort");
					config->tcp = !(p.get<std::string>("Udp", "False") == "True");
					config->maxConnectionAmount = 1;
					m_impl->connectionConfigs.push_back(*config);
					m_impl->managerConfigs.push_back(config);
				}
			}

			iter = ptree.find("CrossServers");
			if (ptree.to_iterator(iter) != ptree.end())
			{
				BOOST_FOREACH(const boost::property_tree::ptree::value_type & v, ptree.get_child("CrossServers"))
				{
					const boost::property_tree::ptree& p = v.second;
					SConnectionConfig* config = new SConnectionConfig(pb::CONNECTION_TYPE_CROSS);
					scgl::CopyStringSafely(config->IP, p.get<std::string>("IP").c_str());
					config->port = p.get<unsigned short>("Port");
					config->tcp = true;
					config->maxConnectionAmount = 1;
					m_impl->connectionConfigs.push_back(*config);
					m_impl->managerConfigs.push_back(config);		

				}
			}
			
		}

#ifdef _WIN32
		RegisterTimedFunctor(5, boost::bind(&CNetService::ProcessAllPacket, this), true);
#endif

		RegisterTimedFunctor(60 * 1000, boost::bind(&CNetService::LogPacketStat, this), true);
	}

	CNetService::~CNetService(void)
	{
		delete m_impl;
	}

	bool CNetService::Connect( const char* remoteIP, unsigned short remotePort, pb::EConnectionType type)
	{
		LOG_DEBUG("Net", "Connecting[" << remoteIP << ":" << remotePort << "],type[" << GetConnectionTypeName(type) << "]");
		SConnectionConfig config(type);
		scgl::CopyStringSafely(config.IP, remoteIP);
		config.port = remotePort;
		return m_impl->tcpHandler.Connect(config);
	}

	bool CNetService::Broadcast(pb::EConnectionType type, scgl::Header& header, const scgl::Message* data, bool disableXP)
	{
		static char buffer[MAX_PACKET_SIZE];
		std::vector<ConnectionPtr> tempConnections;
		if (type != pb::CONNECTION_TYPE_UNKNOWN)
		{
			boost::recursive_mutex::scoped_lock lock(m_impl->connections_mutex);
			const ConnectionDetailMap::index<SConnectionType>::type& connectionMap = m_impl->connections.get<SConnectionType>();
			ConnectionDetailMap::index<SConnectionType>::type::iterator itL = connectionMap.lower_bound(type);
			ConnectionDetailMap::index<SConnectionType>::type::iterator itU = connectionMap.upper_bound(type);
			for (ConnectionDetailMap::index<SConnectionType>::type::iterator it = itL; it != itU; ++it)
			{
				tempConnections.push_back(*it);
			}
		}
		else
		{
			boost::recursive_mutex::scoped_lock lock(m_impl->connections_mutex);
			for (ConnectionDetailMap::iterator it = m_impl->connections.begin(); it != m_impl->connections.end(); ++it)
			{
				tempConnections.push_back(*it);
			}
		}

		for (std::vector<ConnectionPtr>::iterator iter = tempConnections.begin(); iter != tempConnections.end(); ++iter)
		{
			ConnectionPtr connection = *iter;
			if (connection != NULL && connection->guid != 0)
			{
				//附加ack
				header.set_acksequencenumber(connection->ackSequenceNumber);

				//附加包序号
				header.set_sequencenumber(++connection->sendSequenceNumber);

				int size = EncodePacketToBuffer(header, data, buffer, sizeof(buffer));
				if (size > 0)
				{
					if (Write(connection, buffer, size, disableXP) && m_impl->displayMessageFunc != NULL)
					{
						(*m_impl->displayMessageFunc)(">>", connection, header, data);
					}

				}	
			}
		}

		return true;
	}

	bool CNetService::Send(Guid guid, scgl::Header& header, const scgl::Message* data, bool disableXP)
	{
		const ConnectionPtr connection = FindConnection(guid);
		if (connection == NULL || connection->guid == 0)
		{
			return false;
		}

		//附加ack
		header.set_acksequencenumber(connection->ackSequenceNumber);

		//附加包序号
		header.set_sequencenumber(++connection->sendSequenceNumber);

		static char buffer[MAX_PACKET_SIZE];
		int size = EncodePacketToBuffer(header, data, buffer, sizeof(buffer));
		if (size <= 0)
		{
			LOG_ERROR("Net", "Send encode error, connection(" << guid << ") has been dropped.");
			return false;
		}

		bool success = Write(connection, buffer, size, disableXP);
		if (success)
		{
			if (m_impl->needStat)
			{
				// 统计发送包数量和流量
				int packetID = header.packetid();
				IntPacketStatisticsMap::iterator it = m_impl->sentStatistics.find(packetID);
				if (it != m_impl->sentStatistics.end())
				{
					++(it->second.amount);
					it->second.sentSize += size;
				}
				else
				{
					m_impl->sentStatistics.insert(std::make_pair(packetID, SPacketStatistics(packetID, size)));
				}
			}
			if (m_impl->displayMessageFunc != NULL)
			{
				(*m_impl->displayMessageFunc)(">>", connection, header, data);
			}	
		}

		return success;
	}

	bool CNetService::Write( const ConnectionPtr connection, char * data, int size, bool disableXP )
	{
		if (connection == NULL || connection->guid == 0 || !connection->inited)
		{
			return false;
		}

#ifdef ENABLE_DUMP_DATA
		std::ostringstream oss;
		oss << "RAW\tTO_GUID\t" << connection->guid
			<< "\tSIZE\t" << size
			<< "\tDATA\t";
		scgl::DumpData(data, size, oss);
		LOG_TRACE_STR("Net", oss.str().c_str());
#endif
		if (size > scgl::MAX_PACKET_SIZE - sizeof(int))
		{
			//超过最大包限制
			LOG_ERROR("Net", "CNetService::Write size > scgl::MAX_PACKET_SIZE - sizeof(int) guid:" << connection->guid);
			Close(connection);
			return false;
		}

		if (connection->config.tcp)
		{
			//disableXP特殊处理
			if (m_impl->encodeFunc != NULL && !disableXP)
			{
				static char buffer[scgl::MAX_PACKET_SIZE];
				size_t xpSize = scgl::MAX_PACKET_SIZE-sizeof(int);
				scgl::EncodeStatus ret = (*m_impl->encodeFunc)(connection, data, size, buffer+sizeof(int), xpSize);
				if (scgl::ENCODESTATUS_SUCCESS == ret)
				{
#ifdef ENABLE_DUMP_DATA
					std::ostringstream oss;
					oss << "ENCODED\tTO_GUID\t" << connection->guid
						<< "\tSIZE\t" << xpSize
						<< "\tDATA\t";
					scgl::DumpData(buffer+sizeof(int), xpSize, oss);
					LOG_TRACE_STR("Net", oss.str().c_str());
#endif
					*(int*)buffer = xpSize;
					char* newBuffer = new char[xpSize + sizeof(int) + sizeof(char)];
					memcpy(newBuffer, buffer, xpSize + sizeof(int));
					memset(newBuffer + xpSize + sizeof(int), xpSize - size, sizeof(char));
					m_impl->tcpHandler.Write(connection, newBuffer, xpSize + sizeof(int) + sizeof(char));
					return true;
				}
				else if (scgl::ENCODESTATUS_ERROR == ret)
				{
					// 被XP模块检测出错误，可能有挂
					// TODO，不要立刻踢掉，间隔几分钟再踢
					LOG_ERROR("Net", "XP encode failed, the connection(" << connection->guid << ") will be dropped!");
					Close(connection);
					return false;
				}
			}

			char* newBuffer = new char[size+sizeof(int)+sizeof(char)];
			memcpy(newBuffer, (const char*)&size, sizeof(int));
			memcpy(newBuffer + sizeof(int), data, size);
			memset(newBuffer + sizeof(int) + size, 0, sizeof(char));
			m_impl->tcpHandler.Write(connection, newBuffer, size+sizeof(int)+sizeof(char));
			return true;
		}
		return false;
	}

	void CNetService::ProcessAllPacket()
	{
		SProcessData data;
		while (m_impl->debugPacketList.try_pop(data)
			|| m_impl->updatePacketList.try_pop(data)
			|| m_impl->dbPacketList.try_pop(data)
			|| m_impl->defaultPacketList.try_pop(data)
			)
		{
			ProcessPacket(data.source, data.headerPtr, data.data);
		}
#ifndef _WIN32
		m_impl->isPosted = false;
#endif		
	}

	int CNetService::Run()
	{
		boost::system::error_code ec;
		m_impl->io_service.run(ec);
		LOG_DEBUG("Net", "CNetService::Run===== ec:" << ec.value());
		return ec.value();
	}

	int CNetService::NonblockingRun()
	{
		boost::system::error_code ec;
		m_impl->io_service.poll(ec);
		return IsActive();
	}

	void CNetService::Shutdown()
	{
		LOG_DEBUG("Net", "Shutting down...");
		std::vector<ConnectionPtr> tempConnections;
		{
			boost::recursive_mutex::scoped_lock lock(m_impl->connections_mutex);
			for (ConnectionDetailMap::iterator it = m_impl->connections.begin(); it != m_impl->connections.end(); ++it)
			{
				tempConnections.push_back(*it);
			}
		}	
		for (std::vector<ConnectionPtr>::iterator iter = tempConnections.begin(); iter != tempConnections.end(); ++iter)
		{
			ConnectionPtr connection = *iter;
			if (connection != NULL && connection->guid != 0)
			{
				CloseByGuid(connection->guid);
			}
		}
		RegisterTimedFunctor(3000, boost::bind(&CNetService::ShutDownCore, this));
	}

	int CNetService::InsertConnection(ConnectionPtr connection)
	{
		if (connection != NULL && connection->guid != 0)
		{
			connection->lastAliveTime = time(NULL);
			const ConnectionPtr prevConnection = FindConnection(connection->guid);
			if (prevConnection != NULL && prevConnection->guid != 0)
			{
				{
					boost::recursive_mutex::scoped_lock lock(m_impl->connections_mutex);
					m_impl->connections.erase(prevConnection->guid);
					m_impl->connections.insert(connection);
					prevConnection->needReconnect = false;
					prevConnection->guid = 0;
				}	
				connection->waitAckQueue.swap(prevConnection->waitAckQueue);
				connection->ackSequenceNumber = prevConnection->ackSequenceNumber;
				connection->sendSequenceNumber = prevConnection->sendSequenceNumber;
				if (prevConnection->socket != NULL)
				{
					boost::system::error_code ec;
					prevConnection->socket->close(ec);
				}
				LOG_ERROR("Net", "abnormal disconnected!!! reconnect success!!!guid:" << connection->guid
					<< ",waitAckQueue:" << connection->waitAckQueue.size()
					<< ",ackSequenceNumber:" << connection->ackSequenceNumber
					<< ",sendSequenceNumber:" << connection->sendSequenceNumber
					);
				return 1;
			}
			else
			{
				boost::recursive_mutex::scoped_lock lock(m_impl->connections_mutex);
				if (m_impl->connections.insert(connection).second)
				{
					return 0;
				}
			}	
		}
		return -1;
	}

	const ConnectionPtr CNetService::FindConnection(Guid guid) const
	{
		if (IsActive())
		{
			boost::recursive_mutex::scoped_lock lock(m_impl->connections_mutex);
			const ConnectionDetailMap::iterator& iter = m_impl->connections.get<SConnectionGuid>().find(guid);
			if (iter != m_impl->connections.end())
			{
				return *iter;
			}
		}
		return ConnectionPtr();
	}

	Guid CNetService::GetGuid() const
	{
		return m_impl->myID;
	}

	void CNetService::SetGuid(scgl::Guid newGuid)
	{
		m_impl->myID = newGuid;
		LOG_DEBUG("Net", "my Net Guid:" << newGuid);
	}

	bool CNetService::Listen()
	{

		BOOST_FOREACH(ConnectionConfigVector::value_type v, m_impl->connectionConfigs)
		{
			if (v.tcp && v.listener)
			{
				m_impl->tcpHandler.Listen(v);
			}
		}

		return true;
	}

	bool CNetService::IsType( scgl::ConnectionPtr connection, pb::EConnectionType type ) const
	{
		if (connection != NULL && connection->guid != 0)
		{
			return connection->connectionType == type;
		}
		return false;
	}

	void CNetService::CloseByGuid(Guid guid)
	{
		const ConnectionPtr connection = FindConnection(guid);
		if (connection == NULL || connection->guid == 0)
		{
			return;
		}

		Close(connection);
	}

	void CNetService::Close( const ConnectionPtr connection )
	{
		if (connection == NULL || connection->guid == 0)
		{
			return;
		}

		if (connection->config.tcp)
		{
			//本端主动关闭的不需要等待重连
			connection->needReconnect = false;
			m_impl->tcpHandler.Close(connection);
		}
	}

	void CNetService::CloseByType(pb::EConnectionType type)
	{
		if (IsActive())
		{
			std::vector<ConnectionPtr> tempConnections;
			{
				boost::recursive_mutex::scoped_lock lock(m_impl->connections_mutex);
				const ConnectionDetailMap::index<SConnectionType>::type& connectionMap = m_impl->connections.get<SConnectionType>();
				ConnectionDetailMap::index<SConnectionType>::type::iterator itL = connectionMap.lower_bound(type);
				ConnectionDetailMap::index<SConnectionType>::type::iterator itU = connectionMap.upper_bound(type);
				for (ConnectionDetailMap::index<SConnectionType>::type::iterator it = itL; it != itU; ++it)
				{
					tempConnections.push_back(*it);
				}
			}		
			for (std::vector<ConnectionPtr>::iterator iter = tempConnections.begin(); iter != tempConnections.end(); ++iter)
			{
				ConnectionPtr connection = *iter;
				if (connection != NULL && connection->guid != 0)
				{
					Close(connection);
				}
			}
		}
	}

	const char* CNetService::GetListenIPByType(pb::EConnectionType type) const
	{
		for (ConnectionConfigVector::const_iterator it = m_impl->connectionConfigs.begin(); it != m_impl->connectionConfigs.end(); ++it)
		{
			if (it->type == type)
			{
				return it->IP;
			}
		}
		return NULL;
	}

	unsigned short CNetService::GetListenPortByType(pb::EConnectionType type) const
	{
		BOOST_FOREACH(ConnectionConfigVector::value_type v, m_impl->connectionConfigs)
		{
			if (v.type == type)
			{
				return v.port;
			}
		}
		return 0;
	}

	pb::EConnectionType CNetService::GetMyServiceType() const
	{
		return m_impl->serviceType;
	}

	pb::EConnectionType CNetService::GetTypeByGuid( scgl::Guid guid ) const
	{
		ConnectionPtr connection = FindConnection(guid);
		if (connection == NULL || connection->guid == 0)
		{
			return pb::CONNECTION_TYPE_UNKNOWN;
		}

		return connection->connectionType;
	}

	bool CNetService::IsActive() const
	{
		return m_impl->isActive;
	}

	void CNetService::SendInitServer(scgl::Guid serverGuid)
	{
		pb::InitServer initServer;
		initServer.set_serverguid(GetGuid());

		BOOST_FOREACH(ConnectionConfigVector::value_type v, m_impl->connectionConfigs)
		{
			pb::ListenConfig* config = NULL;
			if (v.listener)
			{
				config = initServer.add_servers();
				config->set_listenip(v.IP);
				config->set_listenport(v.port);
			}
			else
			{
				config = initServer.add_clients();
			}
			config->set_connectiontype(v.type);
			config->set_maxconnection(v.maxConnectionAmount);
			config->set_udp(!v.tcp);
		}

		static const int packetID = scgl::GetPacketID("PACKET_ID_INIT_SERVER");
		pb::Header h;
		h.set_packetid(packetID);
		Send(serverGuid, h, &initServer, false);
	}

	bool CNetService::GetRemoteIP( scgl::Guid guid, char* ip, int length ) const
	{
		const ConnectionPtr connection = FindConnection(guid);
		if (connection == NULL || connection->guid == 0)
		{
			return false;
		}

		if (connection->config.tcp)
		{
			return m_impl->tcpHandler.GetRemoteIP(connection, ip, length );
		}

		return false;
	}

	void CNetService::ShutDownCore()
	{
		LOG_DEBUG("Net", "ShutDownCore begin...");
		m_impl->isActive = false;
		{
			boost::recursive_mutex::scoped_lock lock(m_impl->connections_mutex);
			m_impl->connections.clear();
		}
		m_impl->tcpHandler.Shutdown(0);
		m_impl->io_service.stop();
		LOG_DEBUG("Net", "ShutDownCore end**");
	}

	void CNetService::ChangeRemoteGuid(scgl::Guid guid, scgl::Guid newGuid)
	{
		ConnectionPtr connection = FindConnection(guid);
		if (connection != NULL && connection->guid != 0)
		{
			boost::recursive_mutex::scoped_lock lock(m_impl->connections_mutex);
			m_impl->connections.erase(guid);
			connection->guid = newGuid;
			m_impl->connections.insert(connection);
		}
	}

	void CNetService::PostPacket(ConnectionPtr source, scgl::Header* header, scgl::Message* data, int priority)
	{
		if (header == NULL)
		{
			delete data;
			return;
		}

		if (source != NULL)
		{
			header->set_sourceguid(source->guid);
			ProcessAck(source, header);
		}
		else
		{
			header->set_sourceguid(GetGuid());
		}

		switch (priority)
		{
		case PACKET_PRIORITY_DEFAULT:
			m_impl->defaultPacketList.push(SProcessData(source, scgl::HeaderPtr(header), data, priority));
			break;
		case PACKET_PRIORITY_UPDATE:
			m_impl->updatePacketList.push(SProcessData(source, scgl::HeaderPtr(header), data, priority));
			break;
		case PACKET_PRIORITY_DB:
			m_impl->dbPacketList.push(SProcessData(source, scgl::HeaderPtr(header), data, priority));
			break;
		case PACKET_PRIORITY_DEBUG:
			m_impl->debugPacketList.push(SProcessData(source, scgl::HeaderPtr(header), data, priority));
			break;
		}

#ifndef _WIN32
		if (!m_impl->isPosted.compare_and_swap(true, false))
		{
			m_impl->io_service.post(boost::bind(&CNetService::ProcessAllPacket, this));	
		}
#endif
	}

	scgl::Message* CNetService::DecodeMessage( int packetID, const void* data, int size )
	{
		const char* pbName = GetPBName(packetID);
		if (pbName != NULL)
		{
			const gp::Descriptor* descriptor = gp::DescriptorPool::generated_pool()->FindMessageTypeByName(pbName);
			if (descriptor != NULL)
			{
				const scgl::Message* prototype = gp::MessageFactory::generated_factory()->GetPrototype(descriptor);
				if (prototype != NULL)
				{
					scgl::Message* message = prototype->New();
					if (message != NULL)
					{
						bool success = message->ParseFromArray(data, size);
						if (!success)
						{
							LOG_ERROR("Net", "Invalid data, pbName:" << pbName << ",size:" << size);
							delete message;
							return NULL;
						}
						return message;
					}
					LOG_ERROR("Net", "Invalid message, pbName:" << pbName);
				}
				LOG_ERROR("Net", "Invalid prototype, pbName:" << pbName);
			}
			LOG_ERROR("Net", "Invalid descriptor, pbName:" << pbName);
		}
		return NULL;
	}

	bool CNetService::DecodePacket( ConnectionPtr connection, int packetSize, const char* headerBuffer, int headerSize, const char* dataBuffer, int dataSize )
	{
		if (connection == NULL || connection->guid == 0)
		{
			return false;
		}

		if (dataBuffer == NULL || dataSize < 0)
		{
			return false;
		}

		scgl::Header* header = new scgl::Header;
		if (!header->ParseFromArray(headerBuffer, headerSize))
		{
			//解析消息头失败
			LOG_ERROR("Net", "decode header failed!!!" << " guid:" << connection->guid);
			delete header;
			return false;
		}

		//预处理接口
		if (m_impl->prepareFunc != NULL)
		{
			if ((*m_impl->prepareFunc)(connection, header, dataBuffer, dataSize))
			{
				ProcessAck(connection, header);
				delete header;
				return true;
			}
		}

		header->set_packetsize(packetSize);
		scgl::Message* message = DecodeMessage(header->packetid(), dataBuffer, dataSize);
		if (message == NULL && dataBuffer != NULL && dataSize > 0)
		{
			LOG_ERROR("Net", "decode data failed!!!" << "guid:" << connection->guid
				<< ",packetSize:" << packetSize << ",headerSize:" << headerSize << ",dataSize:" << dataSize);
			delete header;
			return false;
		}
		else
		{
			PostPacket(connection, header, message, PACKET_PRIORITY_DEFAULT);
		}

		return true;
	}

	int CNetService::DecodeRingBuffer( ConnectionPtr connection )
	{
		if (connection == NULL || connection->guid == 0)
		{
			return 0;
		}

		const char *dataPtr1, *dataPtr2;
		long sizePtr1, sizePtr2;
		long numBytes = RingBuffer_GetReadRegions(connection->readBuffer, connection->readBuffer->bufferSize, (void**)&dataPtr1, &sizePtr1, (void**)&dataPtr2, &sizePtr2);
		if (numBytes <= sizeof(int))
		{
			return 0;
		}

		int dataLen;
		int packetLen;
		if (sizePtr2 == 0)
		{
			dataLen = *(int*)dataPtr1;
			if (dataLen <= 0)
			{
				char ip[32] = { 0 };
				GetRemoteIP(connection->guid, ip, sizeof(ip));
				LOG_ERROR("Net", "DecodeRingBuffer dataLen <= 0 guid:" << connection->guid << ",ip:" << ip);
				Close(connection);
				return 0;
			}
			dataLen += sizeof(char);
			packetLen = sizeof(int) + dataLen;
			if (numBytes < packetLen)
			{
				return 0;
			}
			else
			{
				DecodeData(connection, dataPtr1 + sizeof(int), dataLen);
				return packetLen;
			}
		}
		else
		{
			static char buffer[MAX_PACKET_SIZE];
			bool isSizeWrap = false;
			if (sizePtr1 < sizeof(int))
			{
				isSizeWrap = true;
				memcpy(buffer, dataPtr1, sizePtr1);
				memcpy(buffer+sizePtr1, dataPtr2, sizeof(int)-sizePtr1);
				dataLen = *(int*)buffer;
			}
			else
			{
				dataLen = *(int*)dataPtr1;
			}
			if (dataLen <= 0)
			{
				char ip[32] = { 0 };
				GetRemoteIP(connection->guid, ip, sizeof(ip));
				LOG_ERROR("Net", "DecodeRingBuffer dataLen <= 0 guid:" << connection->guid << ",ip:" << ip);
				Close(connection);
				return 0;
			}
			dataLen += sizeof(char);
			packetLen = sizeof(int) + dataLen;
			if (numBytes < packetLen)
			{
				return 0;
			}
			else
			{
				const char* xpData;
				if (isSizeWrap || sizePtr1 == sizeof(int))
				{
					xpData = dataPtr2 + sizeof(int)-sizePtr1;
				}
				else if (sizePtr1 < packetLen)
				{
					memcpy(buffer, dataPtr1+sizeof(int), sizePtr1-sizeof(int));
					memcpy(buffer+sizePtr1-sizeof(int), dataPtr2, dataLen-sizePtr1+sizeof(int));
					xpData = buffer;
				}
				else
				{
					xpData = dataPtr1 + sizeof(int);
				}
				DecodeData(connection, xpData, dataLen);
				return packetLen;
			}
		}
	}

	int CNetService::DecodeBuffer( ConnectionPtr connection, const char* buffer, int bufferCapacity )
	{
		int packetSize;
		const char* headerBuffer;
		int headerSize;
		const char* dataBuffer;
		int dataSize;
		if (GetPacketSizeFromBuffer(buffer, bufferCapacity, packetSize, headerBuffer, headerSize, dataBuffer, dataSize))
		{
			if (connection != NULL && connection->guid != 0)
			{
				DecodePacket(connection, packetSize, headerBuffer, headerSize, dataBuffer, dataSize);
			}
		}
		else
		{
			//异常数据
			LOG_ERROR("Net", "DecodeBuffer GetPacketSizeFromBuffer error!!!guid:" << (connection != NULL ? connection->guid : 0)
				<< ", bufferCapacity:" << bufferCapacity
				<< ", packetSize:" << packetSize
				<< ", headerSize:" << headerSize
				<< ", dataSize:" << dataSize
				);
		}
		return packetSize;
	}

	void CNetService::ProcessPacket( ConnectionPtr source, HeaderPtr headerPtr, scgl::Message* data )
	{
		static const int disconnectPacketID = GetPacketID("PACKET_SYSTEM_ID_DISCONNECTED");
		static const int gmCloseLuaStatePacketID = GetPacketID("PACKET_ID_GM_CLOSE_LUA_STATE");
		Header& header = *(headerPtr.get());
		const int packetID = header.packetid();
		pb::EConnectionType connectionType = pb::CONNECTION_TYPE_UNKNOWN;
		int dataSize = 0;
		if (data != NULL)
		{
			dataSize = data->ByteSize();
		}

		if (source == NULL)
		{
			if (m_impl->displayMessageFunc != NULL)
			{
				(*m_impl->displayMessageFunc)("==", source, header, data);
			}
			if (packetID == disconnectPacketID)
			{
				pb::DisconnectionNotification* disconnectPacket = boost::polymorphic_downcast<pb::DisconnectionNotification*>(data);
				if (disconnectPacket != NULL)
				{
					scgl::Guid remoteGuid = disconnectPacket->remoteguid();
					ConnectionPtr connection = FindConnection(remoteGuid);
					if (connection != NULL && connection->guid != 0)
					{
						connectionType = connection->connectionType;
						LOG_DEBUG("Net", "The connection(" << remoteGuid
							<< ") of [" << GetConnectionTypeName(connectionType)
							<< "] is disconnected."
							);
						{
							boost::recursive_mutex::scoped_lock lock(m_impl->connections_mutex);
							m_impl->connections.erase(remoteGuid);
						}	
					}
				}
			}
			else if (packetID == gmCloseLuaStatePacketID)
			{
				m_impl->processorList.clear();
				SLuaHelper::Instance().ReleaseMainState();
			}
		}
		else
		{
			if (source->closeStatus == CLOSESTATUS_CLOSE_FULL)
			{
				//再次确认下
				ConnectionPtr connection = FindConnection(source->guid);
				if (connection == NULL || connection->guid == 0)
				{
					//对方确实已经断线了
					if (m_impl->needDelPacket)
					{
						delete data;
					}
					return;
				}
			}
			connectionType = source->connectionType;
			if (m_impl->displayMessageFunc != NULL)
			{
				(*m_impl->displayMessageFunc)("<<", source, header, data);	
			}
		}

		//统计每一个包处理消耗的时间(单位:纳秒)
		bc::high_resolution_clock::time_point start;
		if (m_impl->needStat)
		{
			IntPacketStatisticsMap::iterator packetStat = m_impl->solvedStatistics.find(packetID);
			if (packetStat != m_impl->solvedStatistics.end())
			{
				++(packetStat->second.amount);
			}
			else
			{
				packetStat = m_impl->solvedStatistics.insert(std::make_pair(packetID, SPacketStatistics(packetID))).first;
			}
			if (m_impl->needStatTime)
			{
				start = bc::high_resolution_clock::now();
			}
		}

		for (ProcessorVector::iterator it = m_impl->processorList.begin(); it != m_impl->processorList.end(); ++it)
		{
			(*it)(source, header, data);
		}

		if (m_impl->needStat && m_impl->needStatTime)
		{
			IntPacketStatisticsMap::iterator packetStat = m_impl->solvedStatistics.find(packetID);
			if (packetStat != m_impl->solvedStatistics.end())
			{
				packetStat->second.time += bc::high_resolution_clock::now() - start;
			}
		}

		if (m_impl->needDelPacket)
		{
			delete data;
		}
	}

	bool CNetService::OpenNAT( scgl::Guid guid, const char* IP, unsigned short port )
	{
		(void)port;
		(void)IP;
		const ConnectionPtr connection = FindConnection(guid);
		if (connection != NULL && connection->guid != 0)
		{
			// connection already exist
			if (connection->config.tcp)
			{
				LOG_ERROR("Net", "The connection(" << guid << ") is tcp connection.");
				return false;
			}
			else
			{
				return true;
			}
		}

		return false;
	}

	void CNetService::ActiveUpdateTimer( TimerPtr timer )
	{
		if (timer == NULL)
		{
			timer.reset(new Timer(m_impl->io_service));
			m_impl->lastUpdateTimePoint = bc::high_resolution_clock::now();
		}
		else
		{
			static const int packetID = scgl::GetPacketID("PACKET_SYSTEM_ID_UPDATE");
			scgl::Header* header = new scgl::Header;
			header->set_packetid(packetID);
			pb::Update* message = new pb::Update;
			bc::high_resolution_clock::time_point now = bc::high_resolution_clock::now(); 
			bc::milliseconds deltaTime = bc::duration_cast<bc::milliseconds>(now - m_impl->lastUpdateTimePoint); 
			message->set_delta((google::protobuf::uint32)deltaTime.count());
			PostPacket(ConnectionPtr(), header, message, PACKET_PRIORITY_UPDATE);
			m_impl->lastUpdateTimePoint = now;
		}
		timer->expires_from_now(bc::milliseconds(m_impl->updateInteval));
		timer->async_wait(boost::bind(&CNetService::ActiveUpdateTimer, this, timer));
	}

	void CNetService::PushProcessor( Processor processor )
	{
		m_impl->processorList.push_back(processor);
	}

	void CNetService::RegisterTimedFunctor( int milliseconds, Functor functor, bool repeat, bool iothread)
	{
		if (iothread)
		{
			TimerPtr timer(new Timer(*m_impl->tcpHandler.GetIOService(), bc::milliseconds(milliseconds)));
			timer->async_wait(boost::bind(&CNetService::OnTimer, this, timer, functor, repeat, milliseconds));
		}
		else
		{
			TimerPtr timer(new Timer(m_impl->io_service, bc::milliseconds(milliseconds)));
			timer->async_wait(boost::bind(&CNetService::OnTimer, this, timer, functor, repeat, milliseconds));
		}
	}

	void CNetService::OnTimer( TimerPtr timer, Functor functor, bool repeat, int milliseconds )
	{
		functor();
		if (repeat)
		{
			timer->expires_from_now(bc::milliseconds(milliseconds));
			timer->async_wait(boost::bind(&CNetService::OnTimer, this, timer, functor, repeat, milliseconds));
		}
	}

	void CNetService::SetClockDifferential( int clockDiff )
	{
		m_impl->clockDifferential = clockDiff;
	}

	int CNetService::GetClockDifferential()
	{
		return m_impl->clockDifferential;
	}

	void CNetService::SetNeedDelPacket()
	{
		m_impl->needDelPacket = true;
	}

	void CNetService::SetDisplayMessageProcess( DisplayMessageProcessPtr displayMessage )
	{
		m_impl->displayMessageFunc = displayMessage; 
	}

	void CNetService::SetEnDecodeProcess( EncodeProcessPtr encode, DecodeProcessPtr decode, PrepareProcessPtr prepare, InitConnectionProcessPtr initConnection )
	{
		m_impl->encodeFunc = encode;
		m_impl->decodeFunc = decode;
		m_impl->prepareFunc = prepare;
		m_impl->initConnectionFunc = initConnection;
	}

	void CNetService::DecodeData( ConnectionPtr connection, const char* dataBuf, int dataLen )
	{
		if (connection == NULL || connection->guid == 0)
		{
			return;
		}

		scgl::Guid remoteGuid = connection->guid;
#ifdef ENABLE_DUMP_DATA
		std::ostringstream oss;
		oss << "ENCODED\tFROM_GUID\t" << remoteGuid
			<< "\tSIZE\t" << dataLen
			<< "\tDATA\t";
		scgl::DumpData(dataBuf, dataLen, oss);
		LOG_TRACE_STR("Net", oss.str().c_str());
#endif
		if (m_impl->decodeFunc != NULL)
		{
			static char buffer[scgl::MAX_PACKET_SIZE];
			size_t appSize = scgl::MAX_PACKET_SIZE;
			scgl::DecodeStatus ret = (*m_impl->decodeFunc)(connection, dataBuf, dataLen, buffer, appSize);
			if (scgl::DECODESTATUS_NORMAL != ret)
			{
				if (scgl::DECODESTATUS_SUCCESS == ret)
				{
#ifdef ENABLE_DUMP_DATA
					std::ostringstream oss;
					oss << "RAW\tFROM_GUID\t" << remoteGuid
						<< "\tSIZE\t" << appSize
						<< "\tDATA\t";
					scgl::DumpData(buffer, appSize, oss);
					LOG_TRACE_STR("Net", oss.str().c_str());
#endif
					char paddingNumber = *(char*)(dataBuf + dataLen - sizeof(char));
					DecodeBuffer(connection, buffer, appSize - paddingNumber);
				}
				else if (scgl::DECODESTATUS_ERROR == ret)
				{
					LOG_ERROR("Net", "DecodeData scgl::DECODESTATUS_ERROR guid:" << remoteGuid);
					Close(connection);
				}
				return;
			}
		}
		DecodeBuffer(connection, dataBuf, dataLen - sizeof(char));
	}

	void CNetService::StartUp( bool isServer, bool needPostUpdate, int writeInteval )
	{
		//开启网络监听端口
		Listen();

		m_impl->isServerNet = isServer;
		if (m_impl->managerConfigs.size() > 0)
		{
			for (size_t i = 0; i <m_impl->managerConfigs.size(); i++)
			{
				SConnectionConfig* config = m_impl->managerConfigs[i];
				if (config->tcp)
				{
					m_impl->tcpHandler.Connect(*config);
				}
				else
				{
					LOG_ERROR("Net", "Can't connect manager server.");
				}
				delete config;
			}
			m_impl->managerConfigs.clear();
		}

		if (needPostUpdate)
		{
			//投递update消息
			ActiveUpdateTimer(TimerPtr());
		}

		//开启tcp服务
		m_impl->tcpHandler.Start(isServer, writeInteval);

		//重连超时检测
		RegisterTimedFunctor(10*1000, boost::bind(&CNetService::Heartbeat, this), true, true);

		m_impl->isActive = true;
	}

	void CNetService::SetNoDelay( scgl::Guid guid, bool nodelay )
	{
		const ConnectionPtr connection = FindConnection(guid);
		if (connection != NULL && connection->guid != 0 && connection->config.tcp)
		{
			scgl::set_no_delay(connection, nodelay);
		}
	}

	bool CNetService::RawSend( scgl::Guid guid, scgl::Header& header, const char* data, int len, bool disableXP )
	{
		const ConnectionPtr connection = FindConnection(guid);
		if (connection == NULL || connection->guid == 0)
		{
			return false;
		}

		//附加ack
		header.set_acksequencenumber(connection->ackSequenceNumber);

		//附加包序号
		header.set_sequencenumber(++connection->sendSequenceNumber);

		static char buffer[scgl::MAX_PACKET_SIZE];
		int size = scgl::EncodePacketToBufferEx(header, data, len, buffer, sizeof(buffer));
		if (size <= 0)
		{
			LOG_ERROR("Net", "Send encode error connection(" << guid << ") has been dropped.");
			return false;
		}	

		return Write(connection, buffer, size, disableXP);
	}

	boost::asio::io_service* CNetService::GetWorkService()
	{
		return &m_impl->io_service;
	}

	bool CNetService::InitConnection( ConnectionPtr connection )
	{
		if (m_impl->initConnectionFunc != NULL && connection != NULL)
		{
			return (*m_impl->initConnectionFunc)(connection);
		}
		return true;
	}

	void CNetService::LogPacketStat() const
	{
		if (m_impl->needStat)
		{
			static std::ostringstream oss;
			oss.str("");
			oss << "PENDING_PACKET\t" << m_impl->defaultPacketList.unsafe_size() + m_impl->dbPacketList.unsafe_size() + m_impl->updatePacketList.unsafe_size() + m_impl->debugPacketList.unsafe_size();
			for (IntPacketStatisticsMap::const_iterator it = m_impl->solvedStatistics.begin(); it != m_impl->solvedStatistics.end(); ++it)
			{
				const SPacketStatistics& stat = it->second;
				const char* packetName = scgl::GetPacketName(stat.packetID);
				if (packetName != NULL)
				{
					oss << "\nSOLVED_PACKET\t" << packetName << "\t" << stat.amount << "\t" << stat.time.count();
				}
			}
			LOG_INFO("Profmon", oss.str());
#if defined(_DEBUG) || defined(DEBUG)
			//统计userdata个数和内存数据
			int udataNum;
			double totalMemorySize = GetUdataNum(udataNum);
			if (udataNum > 0)
			{
				LOG_INFO("Net", "userdata num:" << udataNum << ",totalMemorySize:" << totalMemorySize);
			}
#endif			
		}
	}

	void CNetService::EnablePacketTimeStat()
	{
		m_impl->needStatTime = true;
	}

	void CNetService::DisablePacketTimeStat()
	{
		m_impl->needStatTime = false;
	}

	void CNetService::EnablePacketStat()
	{
		m_impl->needStat = true;
	}

	void CNetService::DisablePacketStat()
	{
		m_impl->needStat = false;
	}

	void CNetService::Heartbeat()
	{
		time_t curTime = time(NULL);
		std::vector<ConnectionPtr> tempConnections;
		{
			boost::recursive_mutex::scoped_lock lock(m_impl->connections_mutex);
			for (ConnectionDetailMap::iterator it = m_impl->connections.begin(); it != m_impl->connections.end(); ++it)
			{
				ConnectionPtr connection = *it;
				if (connection != NULL && connection->guid != 0 && connection->config.tcp)
				{
					scgl::Guid removeGuid = connection->guid;
					if (connection->loseTime > 0
						&& connection->closeStatus != CLOSESTATUS_OPEN 
						&& curTime - connection->loseTime >= 60  //断线重连等待时间为60秒
						&& !connection->isClientNode
						)
					{
						LOG_DEBUG("Net", "Heartbeat reconnect timeout!!!guid:" << removeGuid);
						m_impl->tcpHandler.DestoryConnection(connection);
					}
					else if ((connection->connectionType == pb::CONNECTION_TYPE_PLAYER || connection->connectionType == pb::CONNECTION_TYPE_GM) 
						&& curTime - connection->lastAliveTime >= 180)  //客户端暂停等待时间为180秒
					{
						tempConnections.push_back(connection);
					}
					else
					{
						size_t ackQueueSize = connection->waitAckQueue.size();
						if (!m_impl->isServerNet || connection->connectionType == pb::CONNECTION_TYPE_PLAYER)
						{
							if (ackQueueSize >= 2000)
							{
								tempConnections.push_back(connection);
							}
						}
						else
						{
							if (ackQueueSize >= 50000)
							{
								tempConnections.push_back(connection);
							}
						}
					}
				}
			}
		}
		for (std::vector<ConnectionPtr>::iterator iter = tempConnections.begin(); iter != tempConnections.end(); ++iter)
		{
			ConnectionPtr connection = *iter;
			scgl::Guid removeGuid = connection->guid;
			LOG_DEBUG("Net", "Heartbeat error !!!guid:" << removeGuid);
			m_impl->tcpHandler.Close(connection);
		}
	}

	//处理ACK的逻辑
	void CNetService::ProcessAck(ConnectionPtr connection, scgl::Header* header)
	{
		if (connection == NULL || connection->guid == 0)
		{
			return;
		}

		//对方已经确认接受到的包序号
		unsigned int ackSequenceNumber = header->acksequencenumber();
		connection->peer_ackSequenceNumber = ackSequenceNumber;

		//自己确认收到对方的包序号
		connection->ackSequenceNumber = header->sequencenumber();
		if (ackSequenceNumber > 0)
		{
			//释放待确认数据包
			while(!connection->waitAckQueue.empty())
			{
				const SSendBufInfo& buf = connection->waitAckQueue.front();
				if (buf.sequenceNumber <= ackSequenceNumber)
				{
					delete[] buf.data;
					connection->waitAckQueue.pop_front();
				}
				else
				{
					break;
				}
			}
		}

		//刷新存活时间
		static const int keepAliveID = scgl::GetPacketID("PACKET_ID_SYSSTEM_KEEP_ALIVE");
		if (header->packetid() == keepAliveID)
		{
			connection->lastAliveTime = time(NULL);
		}
	}

	void CNetService::Post( Functor f, bool iothread /*= false*/ )
	{
		if (iothread)
		{
			m_impl->tcpHandler.GetIOService()->post(f);
		}
		else
		{
			m_impl->io_service.post(f);
		}
	}

}
