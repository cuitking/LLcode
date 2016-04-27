#ifndef SCGL_TCPHANDLE_H
#define SCGL_TCPHANDLE_H
#pragma once

#include "Types.h"
#include "Win32.h"
#include <boost/asio/ip/tcp.hpp>

namespace pb
{
	enum EConnectionType;
}

namespace scgl
{
	struct SConnectionConfig;
	class CConnection;

	typedef boost::shared_ptr<boost::asio::ip::tcp::acceptor>	AcceptorPtr;

	boost::system::error_code set_no_delay(scgl::ConnectionPtr connection, bool nodelay);

	class CTcpHandler : boost::noncopyable
	{
	public:

		typedef CTcpHandler Self;

		CTcpHandler(INet& net, IConnectionManager& connectionManager);
		~CTcpHandler();

		void Listen(const SConnectionConfig& config);
		void Start(bool isServer, int writeInteval);
		bool Connect(const SConnectionConfig& config);
		bool Write(ConnectionPtr connection, const void* data, int size, unsigned int sequenceNumber = 0);
		void Close(ConnectionPtr connection, bool isPassive = false);
		void Shutdown(int maxWaittingSecond);
		bool GetRemoteIP(ConnectionPtr connection, char* ip, int length ) const;	
		boost::asio::io_service* GetIOService();
		void DestoryConnection(ConnectionPtr connection);

	private:
		void Accept(AcceptorPtr acceptor, const SConnectionConfig& config);
		void OnAccepted(const boost::system::error_code& ec, AcceptorPtr acceptor, ConnectionPtr connection);
		void Connect(ConnectionPtr connection, const boost::asio::ip::tcp::resolver::iterator& endpoint_iterator);
		void Reconnect(ConnectionPtr connection, const boost::asio::ip::tcp::resolver::iterator& endpoint_iterator, TimerPtr timer);

		void OnActiveConnected(const boost::system::error_code& ec, ConnectionPtr connection, const boost::asio::ip::tcp::resolver::iterator& endpoint_iterator);
		void OnConnected(ConnectionPtr connection, bool active);
		void OnWrittenGuid(const boost::system::error_code& ec, ConnectionPtr connection, bool active);
		void OnReadGuid(const boost::system::error_code& ec, ConnectionPtr connection, bool active);
		void OnReadAckSequenceNumber(const boost::system::error_code& ec, ConnectionPtr connection, bool active);

		bool Read(ConnectionPtr connection);
		void OnRead(const boost::system::error_code& ec, ConnectionPtr connection, int bytes_transferred);
		void OnWritten(const boost::system::error_code& ec, ConnectionPtr connection, int bytes_transferred);

	private:
		struct Impl;
		Impl* m_impl;

	private:
		void writeImpl(ConnectionPtr connection);
		bool write_core(ConnectionPtr connection);
		void closeImpl(ConnectionPtr connection, bool isPassive);
		void close_core(ConnectionPtr connection);
		void ActiveWriteTimer(TimerPtr timer);
		bool IsWriteFast();
	};

}
#endif
