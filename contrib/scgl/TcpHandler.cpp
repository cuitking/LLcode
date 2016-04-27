#include "TcpHandler.h"
#include "Net.h"
#include "ConnectionManager.h"
#include "Connection.h"
#include "ConnectionConfig.h"
#include "Packet.h"
#include "Log.h"
#include "capi.h"
#include "StringAlgorithm.h"
#include "Utility.h"
#include "../pb/header.pb.h"
#include "../pb/packetSystem.pb.h"
#include <tbb/concurrent_queue.h>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem/fstream.hpp>

using namespace boost::asio;

namespace
{
	boost::system::error_code apply_socket_options(scgl::CConnection& connection)
	{
		const scgl::SConnectionConfig& config = connection.config;

		boost::system::error_code error;

		if (config.writeBufferCapacity != 0)
		{
			boost::asio::socket_base::send_buffer_size option(config.writeBufferCapacity);
			connection.socket->set_option(option, error);
			if (error)
			{
				return error;
			}
		}

		if (config.readBufferCapacity != 0)
		{
			boost::asio::socket_base::receive_buffer_size option(config.readBufferCapacity);
			connection.socket->set_option(option, error);
			if (error)
			{
				return error;
			}
		}

		connection.socket->non_blocking(true, error);

		return error;
	}

	bool IsError(const char* functionName, const boost::system::error_code& ec, scgl::ConnectionPtr connection)
	{
		if (!ec)
		{
			return false;
		}

		if (connection != NULL && connection->guid != 0)
		{
			if (ec != boost::asio::error::eof)
			{
				LOG_DEBUG("Net", "The connection(type[" << scgl::GetConnectionTypeName(connection->connectionType)
					<< "] guid[" << connection->guid
					<< "]) got a ec(no[" << ec
					<< "] message[" << NativeToUTF8(boost::system::system_error(ec).what())
					<< "] function[" << functionName
					<< "]."
				);
			}
			else
			{
				LOG_DEBUG("Net", "The connection(type[" << scgl::GetConnectionTypeName(connection->connectionType)
					<< "] guid[" << connection->guid
					<< "] function[" << functionName << "]) closed gracefully."
				);
			}
		}
		else
		{
			LOG_ERROR("Net", functionName
				<< " Error(" << ec
				<< "),"
				<< boost::system::system_error(ec).what()
				<< "."
			);
		}
		return true;
	}
}

namespace scgl
{
	typedef tbb::concurrent_queue<ConnectionPtr>	WriteDeque;
	typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
	typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;

	boost::system::error_code set_no_delay(scgl::ConnectionPtr connection, bool nodelay)
	{
		boost::system::error_code error;
		if (connection != NULL)
		{
			boost::asio::ip::tcp::no_delay option(nodelay);
			connection->socket->set_option(option, error);
		}
		return error;
	}

	struct CTcpHandler::Impl
	{
		INet&							net;
		IConnectionManager&				connectionManager;

		io_service_ptr					accept_service;
		io_service_ptr					io_service;

		/// The work that keeps the io_services running.
		std::vector<work_ptr> work_;
		boost::shared_ptr<boost::thread> acceptThread;
		boost::shared_ptr<boost::thread> ioThread;

		WriteDeque						sendQueue;
		int								writeInteval;
		bool							isServerNet;

		Impl(INet& _net, IConnectionManager& _connectionManager)
			: net(_net)
			, connectionManager(_connectionManager)
			, accept_service(new boost::asio::io_service)
			, io_service(new boost::asio::io_service)
			, writeInteval(0)
			, isServerNet(false)
		{
			work_ptr accept_work(new boost::asio::io_service::work(*accept_service));
			work_.push_back(accept_work);
			work_ptr io_work(new boost::asio::io_service::work(*io_service));
			work_.push_back(io_work);
		}

		~Impl()
		{
		}
	};

	void CTcpHandler::Listen( const SConnectionConfig& config )
	{
		if (m_impl->accept_service->stopped())
		{
			LOG_ERROR("Net", __FUNCTION__ << " error, the service is stopped.");
			return;
		}

		// 允许监听所有网卡或者指定网卡
		boost::asio::ip::tcp::endpoint local(boost::asio::ip::tcp::v4(), config.port);
		if (config.IP[0] != '\0')
		{
			local = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(config.IP), config.port);
		}

		boost::asio::ip::address address = local.address();
		unsigned short port = local.port();

		AcceptorPtr acceptor(new boost::asio::ip::tcp::acceptor(*m_impl->accept_service));
		acceptor->open(local.protocol());
		acceptor->set_option(socket_base::reuse_address(true));
		boost::system::error_code ec;
		acceptor->bind(local, ec);
		if (ec)
		{
			// An error occurred.
			LOG_ERROR("Net", "bind error[" << ec << "] at [" << address.to_string() << ":" << port << "], type:" << GetConnectionTypeName(config.type));
			exit(-1);
		}
		acceptor->listen(boost::asio::socket_base::max_connections, ec);
		if (ec)
		{
			// An error occurred.
			LOG_ERROR("Net", "listen error[" << ec << "] at [" << address.to_string() << ":" << port << "], type:" << GetConnectionTypeName(config.type));
			exit(-1);
		}

		LOG_DEBUG("Net", "listen Success at [" << address.to_string() << ":" << port << "], type:" << GetConnectionTypeName(config.type));
		Accept(acceptor, config);
	}

	void CTcpHandler::Accept( AcceptorPtr acceptor, const SConnectionConfig& config )
	{
		if (m_impl->accept_service->stopped())
		{
			LOG_ERROR("Net", __FUNCTION__ << " error, the service is stopped.");
			return;
		}

		ConnectionPtr connection(new CConnection(config, m_impl->io_service.get()));
		acceptor->async_accept(*(connection->socket), boost::bind(&Self::OnAccepted, this, boost::asio::placeholders::error
			, acceptor, connection));
	}

	// 收到连接成功
	void CTcpHandler::OnAccepted( const boost::system::error_code& ec, AcceptorPtr acceptor, ConnectionPtr connection )
	{
		if (IsError(__FUNCTION__, ec, connection))
		{
			boost::system::error_code ec;
			connection->socket->close(ec);
			return;
		}
		OnConnected(connection, false);
		Accept(acceptor, connection->config);
	}

	bool CTcpHandler::Connect( const SConnectionConfig& config )
	{
		if (m_impl->io_service->stopped())
		{
			LOG_ERROR("Net", __FUNCTION__ << " error, the service is stopped.");
			return false;
		}

		char strPort[16];
		boost::system::error_code ec;
		sprintf(strPort, "%u", config.port);
		boost::asio::ip::tcp::resolver resolver(*m_impl->io_service);
		boost::asio::ip::tcp::resolver::query query(config.IP, strPort);
		boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, ec);
		boost::asio::ip::tcp::resolver::iterator end;

		if (IsError(__FUNCTION__, ec, ConnectionPtr()))
		{
			//异常情况
			return false;
		}

		if (endpoint_iterator == end)
		{
			//域名解析失败
			return false;
		}

		ConnectionPtr connection(new CConnection(config, m_impl->io_service.get(), true));
		Connect(connection, endpoint_iterator);
		return true;
	}

	void CTcpHandler::Connect( ConnectionPtr connection, const boost::asio::ip::tcp::resolver::iterator& endpoint_iterator )
	{
		boost::asio::async_connect((*connection->socket), endpoint_iterator,
			boost::bind(&Self::OnActiveConnected, this, boost::asio::placeholders::error, connection, endpoint_iterator));
	}

	void CTcpHandler::OnActiveConnected( const boost::system::error_code& ec, ConnectionPtr connection, const boost::asio::ip::tcp::resolver::iterator& endpoint_iterator )
	{
		if (IsError(__FUNCTION__, ec, connection))
		{
			if (connection->config.autoReConnect && ++connection->retryCount <= 5)
			{
				//重连等待时间递增
				TimerPtr t(new Timer(*m_impl->io_service, boost::chrono::seconds(connection->retryCount * 5)));
				t->async_wait(boost::bind(&Self::Reconnect, this, connection, endpoint_iterator, t));
				LOG_ERROR("Net", "Re-Connecting type[" << GetConnectionTypeName(connection->config.type) << "]");
			}
			else
			{
				//尝试连接失败，放弃。。。
				static const int packetID = GetPacketID("PACKET_SYSTEM_ID_CONNECTION_ATTEMPT_FAILED");
				scgl::Header* header = new scgl::Header;
				header->set_packetid(packetID);
				pb::ConnectionAttemptFailed* data = new pb::ConnectionAttemptFailed;
				data->set_ip(connection->config.IP);
				data->set_port(connection->config.port);
				data->set_connectiontype(connection->connectionType);
				m_impl->net.PostPacket(ConnectionPtr(), header, data);
			}
		}
		else
		{
			OnConnected(connection, true);
		}
	}

	void CTcpHandler::Reconnect( ConnectionPtr connection, const boost::asio::ip::tcp::resolver::iterator& endpoint_iterator, TimerPtr timer )
	{
		//继续尝试连接
		Connect(connection, endpoint_iterator);
	}

	void CTcpHandler::OnConnected( ConnectionPtr connection, bool active )
	{
		if(apply_socket_options(*connection))
		{
			char ip[32] = { 0 };
			GetRemoteIP(connection, ip, sizeof(ip));
			LOG_ERROR("Net", "OnConnected error!!! ip:" << ip);
			boost::system::error_code ec;
			connection->socket->close(ec);
			return;
		}

		if (!m_impl->isServerNet 
			|| (connection->connectionType == pb::CONNECTION_TYPE_PLAYER || connection->connectionType == pb::CONNECTION_TYPE_GM)
			)
		{
			connection->CreateBuffer(MAX_RECVBUFFER);
		}
		else
		{
			//服务器之间开启nodelay
			set_no_delay(connection, true);
			connection->CreateBuffer(SERVER_SERVER_BUFFER);
		}

		//TCP连接上了之后发送自己的Guid
		Guid guid = m_impl->net.GetGuid();
		boost::system::error_code ec;
		boost::asio::write(*(connection->socket), 
			boost::asio::buffer(&guid, sizeof(Guid)),
			boost::asio::transfer_all(),
			ec
		);

		if (ec)
		{
			boost::system::error_code ec;
			connection->socket->close(ec);
			return;
		}

		//guid读取到connection->guid上
		boost::asio::async_read(*(connection->socket),
			boost::asio::buffer(&connection->guid, sizeof(connection->guid)),
			boost::asio::transfer_all(),
			boost::bind(&Self::OnReadGuid,
			this,
			boost::asio::placeholders::error,
			connection,
			active
			)
		);
	}

	void CTcpHandler::OnReadGuid( const boost::system::error_code& ec, ConnectionPtr connection, bool active )
	{
		if (IsError(__FUNCTION__, ec, connection))
		{
			boost::system::error_code ec;
			connection->socket->close(ec);
			return;
		}

		int ret = m_impl->connectionManager.InsertConnection(connection);
		if (ret == -1)
		{
			Close(connection);
			return;
		}
		else if (ret == 0)
		{
			if(!m_impl->net.InitConnection(connection))
			{
				Close(connection);
				return;
			}
		}

		//TCP连接上了之后发送自己接收到确认包
		boost::system::error_code errorCode;
		boost::asio::write(*(connection->socket), 
			boost::asio::buffer(&connection->ackSequenceNumber, sizeof(unsigned int)),
			boost::asio::transfer_all(),
			errorCode
		);

		if (errorCode)
		{
			Close(connection);
			return;
		}

		//guid读取到connection->peer_ackSequenceNumber上
		boost::asio::async_read(*(connection->socket),
			boost::asio::buffer(&connection->peer_ackSequenceNumber, sizeof(connection->peer_ackSequenceNumber)),
			boost::asio::transfer_all(),
			boost::bind(&Self::OnReadAckSequenceNumber,
			this,
			boost::asio::placeholders::error,
			connection,
			active
			)
		);
	}

	void CTcpHandler::OnReadAckSequenceNumber( const boost::system::error_code& ec, ConnectionPtr connection, bool active )
	{
		if (connection == NULL)
		{
			return;
		}

		if (IsError(__FUNCTION__, ec, connection))
		{
			Close(connection);
			return;
		}

		unsigned int ackSequenceNumber = connection->peer_ackSequenceNumber;
		boost::system::error_code error;

		boost::asio::ip::tcp::endpoint local = connection->socket->local_endpoint(error);
		if (error)
		{
			LOG_ERROR("Net", __FUNCTION__
				<< " Error(" << error
				<< "),"
				<< boost::system::system_error(error).what()
				<< "."
				);
			Close(connection);
			return;
		}

		boost::asio::ip::tcp::endpoint remote = connection->socket->remote_endpoint(error);
		if (error)
		{
			LOG_ERROR("Net", __FUNCTION__
				<< " Error(" << error
				<< "),"
				<< boost::system::system_error(error).what()
				<< "."
				);
			Close(connection);
			return;
		}

		LOG_DEBUG("Net", "Connected type:" << GetConnectionTypeName(connection->connectionType)
			<< ", from [" << remote.address().to_string() << ":" << remote.port()
			<< "] at [" << local.address().to_string() << ":" << local.port() 
			<< "] guid [" << connection->guid
			<< "] ackSequenceNumber [" << ackSequenceNumber << "]."
		);
		
		if (ackSequenceNumber > 0)
		{
			//重连逻辑处理
			size_t n = connection->waitAckQueue.size();
			for (size_t i = 0; i < n; ++i)
			{
				const SSendBufInfo& buf = connection->waitAckQueue.front();
				if (buf.sequenceNumber <= ackSequenceNumber)
				{
					delete[] buf.data;
				}
				else
				{
					//重发一次离线信息
					LOG_ERROR("Net", "re-Write guid:" << connection->guid << ",sequenceNumber:" << buf.sequenceNumber);
					Write(connection, buf.data, buf.len, buf.sequenceNumber);
				}
				connection->waitAckQueue.pop_front();
			}
		}
		else
		{
			//正常连接逻辑处理
			connection->inited = true;
			if (active)
			{
				pb::ConnectionRequestAccepted* data = new pb::ConnectionRequestAccepted;
				data->set_connectiontype(connection->connectionType);
				data->set_remoteguid(connection->guid);
				static const int requestAccept = GetPacketID("PACKET_SYSTEM_ID_REQUEST_ACCEPTED");
				scgl::Header* header = new scgl::Header;
				header->set_packetid(requestAccept);
				m_impl->net.PostPacket(ConnectionPtr(), header, data);
			}
			else
			{
				pb::NewIncomingConnection* data = new pb::NewIncomingConnection;
				data->set_connectiontype(connection->connectionType);
				data->set_remoteguid(connection->guid);
				static const int newIncoming = GetPacketID("PACKET_SYSTEM_ID_NEW_INCOMING");
				scgl::Header* header = new scgl::Header;
				header->set_packetid(newIncoming);
				m_impl->net.PostPacket(ConnectionPtr(), header, data);
			}
		}

		Read(connection);
	}

	bool CTcpHandler::Read( ConnectionPtr connection )
	{
		if (connection == NULL)
		{
			return false;
		}

		void *dataPtr1, *dataPtr2;
		long sizePtr1, sizePtr2;
		long numBytes = RingBuffer_GetWriteRegions(connection->readBuffer, connection->readBuffer->bufferSize, &dataPtr1, &sizePtr1, &dataPtr2, &sizePtr2);
		if (numBytes == 0)
		{
			return false;
		}
		else
		{
			connection->socket->async_read_some(
				boost::asio::buffer(dataPtr1, sizePtr1),
				boost::bind(&Self::OnRead,
				this,
				boost::asio::placeholders::error,
				connection,
				boost::asio::placeholders::bytes_transferred
				)
			);
			return true;
		}
	}

	void CTcpHandler::OnRead( const boost::system::error_code& ec, ConnectionPtr connection, int bytes_transferred )
	{
		if (connection == NULL)
		{
			return;
		}

		if (IsError(__FUNCTION__, ec, connection))
		{
			if (ec == boost::asio::error::eof  //2:正常关闭
				|| ec == boost::asio::error::connection_reset //10054:对方强行关闭链路
			)
			{
				connection->needReconnect = false;
			}
			else //其他网络错误
			{
				connection->needReconnect = true;
			}
			if (connection->guid != 0)
			{
				if (connection->closeStatus == CLOSESTATUS_CLOSE_WRITE)
				{
					close_core(connection);
				}
				else
				{
					Close(connection, true);
				}
			}	
			return;
		}

		//如果发现对端链路异常或者双方完全关闭
		if (connection->closeStatus == CLOSESTATUS_2_CLOSING || connection->closeStatus == CLOSESTATUS_CLOSE_FULL)
		{
			return;
		}

		RingBuffer_AdvanceWriteIndex(connection->readBuffer, bytes_transferred);
		bool ret = Read(connection);
		int processed;
		while ((processed = m_impl->net.DecodeRingBuffer(connection)) > 0)
		{
			RingBuffer_AdvanceReadIndex(connection->readBuffer, processed);
		}
		if (!ret)
		{
			//如果之前缓冲区满了的话，现在重新投递read
			ret = Read(connection);
			if (!ret)
			{
				//数据异常
				const char *dataPtr1, *dataPtr2;
				long sizePtr1, sizePtr2;
				long numBytes = RingBuffer_GetReadRegions(connection->readBuffer, connection->readBuffer->bufferSize, (void**)&dataPtr1, &sizePtr1, (void**)&dataPtr2, &sizePtr2);
				LOG_ERROR("Net", "CTcpHandler::OnRead data full!!!guid:" << connection->guid 
					<< ",bufferSize:" << connection->readBuffer->bufferSize 
					<< ",sizePtr1:" << sizePtr1 << ",sizePtr2:" << sizePtr2 
					<< ",numBytes:" << numBytes
					);
				Close(connection);
			}
		}
	}

	bool CTcpHandler::Write( ConnectionPtr connection, const void* data, int size, unsigned int sequenceNumber /* = 0 */ )
	{
		if (connection == NULL)
		{
			delete[] (char*)data;
			return false;
		}

		SSendBufInfo buf;
		buf.data = (char*)data;
		buf.len = size;
		if (sequenceNumber == 0)
		{
			buf.sequenceNumber = connection->sendSequenceNumber;
		}
		else
		{
			buf.sequenceNumber = sequenceNumber;
		}
		connection->sendQueue.push(buf);
		if (!connection->writting.compare_and_swap(true, false))
		{
			if (!IsWriteFast())
			{
				m_impl->sendQueue.push(connection);
			}
			else
			{
				m_impl->io_service->post(boost::bind(&Self::writeImpl, this, connection));
			}
		}
		return true;
	}

	bool CTcpHandler::write_core( ConnectionPtr connection )
	{
		if (connection == NULL)
		{
			return false;
		}

		//临时排序map
		std::map<unsigned int, SSendBufInfo> sortSendMap;
		std::vector<boost::asio::const_buffer> buffers;
		SSendBufInfo tmpBuf;
		while (connection->sendQueue.try_pop(tmpBuf))
		{
			sortSendMap.insert(std::make_pair(tmpBuf.sequenceNumber, tmpBuf));
		}
		for (std::map<unsigned int, SSendBufInfo>::iterator iter = sortSendMap.begin(); iter != sortSendMap.end(); ++iter)
		{
			const SSendBufInfo& buf = iter->second;
			connection->waitAckQueue.push_back(buf);
			buffers.push_back(boost::asio::buffer(buf.data, buf.len));
		}
		if (buffers.size() > 0)
		{
			boost::asio::async_write(*(connection->socket),
				buffers,
				boost::bind(&CTcpHandler::OnWritten,
				this,
				boost::asio::placeholders::error,
				connection,
				boost::asio::placeholders::bytes_transferred
				)
			);
			return true;
		}
		return false;
	}

	void CTcpHandler::OnWritten( const boost::system::error_code& ec, ConnectionPtr connection, int /*bytes_transferred*/ )
	{
		if (connection == NULL)
		{
			return;
		}

		if (IsError(__FUNCTION__, ec, connection))
		{
			//对端网络异常
			connection->writting.compare_and_swap(false, true);
			connection->closeStatus = CLOSESTATUS_CLOSE_WRITE;
			close_core(connection);
		}
		else
		{
			writeImpl(connection);
		}
	}

	void CTcpHandler::close_core( ConnectionPtr connection )
	{
		if (connection == NULL)
		{
			return;
		}

		//关闭socket连接
		boost::system::error_code ec;
		connection->socket->close(ec);

		LOG_DEBUG("Net", "CTcpHandler::close_core type:" << GetConnectionTypeName(connection->connectionType)
			<< " guid:" << connection->guid
			);

		connection->closeStatus = CLOSESTATUS_CLOSE_FULL;

		if (connection->needReconnect)
		{
			//对于服务器来说等待对方重连
			connection->loseTime = time(NULL);
			if (connection->isClientNode)
			{
				//对于客户端来说直接发起连接
				LOG_DEBUG("Net", "peer abnormal disconnection!!!reconnect...guid:" << connection->guid);
				SConnectionConfig newConfig = connection->config;
				newConfig.autoReConnect = true;
				Connect(newConfig);
			}
			else
			{
				LOG_DEBUG("Net", "peer abnormal disconnection!!!wait reconnect...guid:" << connection->guid);
			}
		}
		else
		{
			DestoryConnection(connection);
		}
	}

	void CTcpHandler::Close( ConnectionPtr connection, bool isPassive )
	{
		if (connection == NULL)
		{
			return;
		}

		LOG_DEBUG("Net", "CTcpHandler::Close type:" << GetConnectionTypeName(connection->connectionType)
			<< " guid:" << connection->guid
			<< " isPassive:" << isPassive
			);

		m_impl->io_service->post(boost::bind(&Self::closeImpl, this, connection, isPassive));
	}

	void CTcpHandler::Shutdown(int maxWaittingSecond)
	{
		(void)maxWaittingSecond;
		m_impl->accept_service->stop();
		m_impl->io_service->stop();
		m_impl->work_.clear();
		m_impl->acceptThread->join();
		m_impl->ioThread->join();
	}

	CTcpHandler::CTcpHandler(INet& net, IConnectionManager& connectionManager)
		: m_impl(new Impl(net, connectionManager))
	{
	}

	CTcpHandler::~CTcpHandler()
	{
		delete m_impl;
	}

	bool CTcpHandler::GetRemoteIP( ConnectionPtr connection, char* ip, int length ) const
	{
		if (connection == NULL)
		{
			return false;
		}

		boost::system::error_code ec;
		boost::asio::ip::tcp::endpoint remoteAddress = connection->socket->remote_endpoint(ec);
		if (!ec)
		{
			std::string address = remoteAddress.address().to_string();
			int copySize = scgl::CopyMemorySafely(ip, length - 1, address.c_str(), address.length());
			ip[length > copySize ? copySize : length - 1] = '\0';
			return true;
		}
		return false;
	}

	void CTcpHandler::Start(bool isServer, int writeInteval)
	{
		m_impl->isServerNet = isServer;
		m_impl->writeInteval = writeInteval;
		m_impl->acceptThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&boost::asio::io_service::run, m_impl->accept_service)));
		m_impl->ioThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&boost::asio::io_service::run, m_impl->io_service)));
		if (!IsWriteFast())
		{
			m_impl->writeInteval = writeInteval;
			ActiveWriteTimer(TimerPtr());
		}
	}

	void CTcpHandler::writeImpl( ConnectionPtr connection )
	{
		if (connection == NULL)
		{
			return;
		}

		if (connection->closeStatus == CLOSESTATUS_OPEN ||
			connection->closeStatus == CLOSESTATUS_1_CLOSING || 
			connection->closeStatus == CLOSESTATUS_2_CLOSING)
		{
			if (!write_core(connection))
			{
				connection->writting.compare_and_swap(false, true);
				if (connection->closeStatus == CLOSESTATUS_1_CLOSING)
				{
					LOG_DEBUG("Net", "CTcpHandler::shutdown_send type:" << GetConnectionTypeName(connection->connectionType)
						<< " guid:" << connection->guid
					);
					boost::system::error_code ec;
					connection->socket->shutdown(boost::asio::socket_base::shutdown_send, ec);
					connection->closeStatus = CLOSESTATUS_CLOSE_WRITE;	//等待对方关闭发送链路
					if (ec)
					{
						//异常情况
						LOG_DEBUG("Net", "socket::shutdown error:" << ec
							<< " guid:" << connection->guid
							);
						close_core(connection);
					}
				}
				else if (connection->closeStatus == CLOSESTATUS_2_CLOSING)
				{
					close_core(connection);
				}
			}
		}
	}

	void CTcpHandler::closeImpl( ConnectionPtr connection, bool isPassive )
	{
		if (connection == NULL)
		{
			return;
		}

		LOG_DEBUG("Net", "CTcpHandler::closeImpl type:" << GetConnectionTypeName(connection->connectionType)
			<< " guid:" << connection->guid
			<< " isPassive:" << isPassive
		);

		if (connection->closeStatus == CLOSESTATUS_CLOSE_WRITE)
		{
			close_core(connection);
		}
		else
		{
			if (!isPassive)
			{
				connection->closeStatus = CLOSESTATUS_1_CLOSING;
			}
			else
			{
				connection->closeStatus = CLOSESTATUS_2_CLOSING;
			}	
			if (!connection->writting.compare_and_swap(true, false))
			{
				if (!IsWriteFast())
				{
					m_impl->sendQueue.push(connection);
				}
				else
				{
					m_impl->io_service->post(boost::bind(&Self::writeImpl, this, connection));
				}
			}
		}
	}

	void CTcpHandler::ActiveWriteTimer( TimerPtr timer )
	{
		if (timer == NULL)
		{
			timer.reset(new Timer(*m_impl->io_service));
		}
		else
		{
			ConnectionPtr connection;
			while (m_impl->sendQueue.try_pop(connection))
			{
				if (connection != NULL)
				{
					writeImpl(connection);
				}
			}
		}	
		timer->expires_from_now(boost::chrono::milliseconds(m_impl->writeInteval));
		timer->async_wait(boost::bind(&CTcpHandler::ActiveWriteTimer, this, timer));
	}

	bool CTcpHandler::IsWriteFast()
	{
		return !(m_impl->isServerNet && m_impl->writeInteval > 0);
	}

	boost::asio::io_service* CTcpHandler::GetIOService()
	{
		return m_impl->io_service.get();
	}

	void CTcpHandler::DestoryConnection( ConnectionPtr connection )
	{
		if (connection == NULL)
		{
			return;
		}
		if (connection->guid != 0)
		{
			pb::DisconnectionNotification* data = new pb::DisconnectionNotification;
			data->set_connectiontype(connection->connectionType);
			data->set_remoteguid(connection->guid);
			data->set_bpassive(false);
			static const int packetID = GetPacketID("PACKET_SYSTEM_ID_DISCONNECTED");
			scgl::Header* header = new scgl::Header;
			header->set_packetid(packetID);
			header->set_sourceguid(connection->guid);
			m_impl->net.PostPacket(ConnectionPtr(), header, data);
		}	
	}

}
