#include "DBServer.h"
#include "DBEvent.h"
#include <mongo/client/dbclient.h>
#include <memory>
#include <string>
#include <iostream>
#include <boost/threadpool.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

DEFINITION_SINGLETON(SDBServer)

typedef boost::shared_ptr<boost::threadpool::pool> ThreadPool;
typedef struct connect_data_s
{
	//异步IO线程
	ThreadPool threadPool;
}connect_data;
typedef std::map<void*, connect_data> ConnectionPool;

struct SDBServer::Impl
{
	ConnectionPool connectionPool;
#ifdef _DEBUG
	int sleepTime;
	Impl()
		: sleepTime(0)
	{
	}
#endif
};

void HandleAsyncCall(CDBEvent* pap)
{
	if (pap != NULL)
	{
#ifdef _DEBUG
		int sleepTime = SDBServer::Instance().GetSleepTime();
		if (sleepTime > 0)
		{
			Sleep(sleepTime);
		}
#endif
		pap->Do_Excute();
	}
}

SDBServer::SDBServer(void)
	: m_impl(new Impl)
{
}

SDBServer::~SDBServer(void)
{
	delete m_impl;
}

bool SDBServer::AsyncProcess(void* connection, CDBEvent* pEvent)
{
	ConnectionPool::iterator iter = m_impl->connectionPool.find(connection);
	if (iter != m_impl->connectionPool.end())
	{
		connect_data& data = iter->second;
		data.threadPool->schedule(boost::bind(&HandleAsyncCall, pEvent));
		return true;
	}
	return false;
}

void SDBServer::AddConnection(void* connection)
{
	connect_data data;
	data.threadPool = ThreadPool(new boost::threadpool::pool(1));
	m_impl->connectionPool.insert(std::make_pair(connection, data));
}

void SDBServer::UnInit()
{
	ConnectionPool::iterator iter = m_impl->connectionPool.begin();
	for (; iter != m_impl->connectionPool.end(); ++iter)
	{
		connect_data& data = iter->second;
		// Wait until all tasks are finished.
		data.threadPool->wait();
	}
	m_impl->connectionPool.clear();
}

#ifdef _DEBUG
int SDBServer::GetSleepTime()
{
	return m_impl->sleepTime;
}

void SDBServer::SetSleepTime( int sleepTime )
{
	m_impl->sleepTime = sleepTime;
}
#endif
