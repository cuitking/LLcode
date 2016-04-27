#include "DBEvent.h"
#include "common.h"
#include "utils.h"
#include <mongo/client/dbclient.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <string>

extern void Handle_Query(ULONG_PTR dwParam);
extern void Handle_Insert(ULONG_PTR dwParam);
extern void Handle_Update(ULONG_PTR dwParam);
extern void Handle_Remove(ULONG_PTR dwParam);
extern void Handle_Command(ULONG_PTR dwParam);
extern void Handle_Count(ULONG_PTR dwParam);

extern void bson_to_lua(lua_State* L, const mongo::BSONObj& obj);
boost::asio::io_service* work_service;

const mongo::BSONObj normalCmdObj = mongo::fromjson("{getlasterror:1}");
const mongo::BSONObj hardCmdObj = mongo::fromjson("{getlasterror:1,j:true}");
const mongo::BSONObj repl_hardCmdObj = mongo::fromjson("{getlasterror:1,j:true,w:'majority',wtimeout:5000}");

void mongo_getLastError(mongo::DBClientConnection* connection, int writeConcern, mongo::BSONObj& retInfo)
{
	if (writeConcern == 1)
	{
		connection->runCommand("admin", normalCmdObj, retInfo);
	}
	else if (writeConcern == 2)
	{
		connection->runCommand("admin", hardCmdObj, retInfo);
	}
}

CDBEvent::~CDBEvent()
{

}

bool CDBEvent::GetResult( mongo::BSONObj& retInfo )
{
	return true;
}
//////////////////////////////////////////////////////////

template<>
struct Loki::ImplOf<CDBQueryEvent>
{
	mongo::DBClientConnection* connection;
	const char* ns;
	mongo::Query* queryObj;
	int nToReturn;
	int nToSkip;
	const mongo::BSONObj* fieldsToReturn;
	bool nonBlock;
	lua_State* m_thread;
	std::vector<mongo::BSONObj> results;
	int event;
};

std::vector<mongo::BSONObj>& CDBQueryEvent::GetResult()
{
	return m_impl->results;
}

void CDBQueryEvent::Do_Excute()
{
	try
	{
		m_impl->results.clear();
		std::auto_ptr<mongo::DBClientCursor> autocursor = m_impl->connection->query(
					m_impl->ns, *m_impl->queryObj, m_impl->nToReturn, m_impl->nToSkip,
					m_impl->fieldsToReturn);
		mongo::DBClientCursor* cursor = autocursor.get();
		if (cursor != NULL)
		{
			const static int Max_Number = 5000;
			while (cursor->more())
			{
				m_impl->results.push_back(cursor->next().copy());
			}
			if (m_impl->nonBlock && m_impl->results.size() > Max_Number) //非阻塞查询需要限制最大记录数
			{
				//异常情况
				FILE* fp = fopen("sql_error.txt", "a");
				if (fp != NULL)
				{
					fprintf(fp, "CDBQueryEvent error!!!condition:%s\n", m_impl->queryObj->toString().c_str());
					fclose(fp);
				}
				m_impl->results.clear();
			}
		}
	}
	catch (...)
	{
		m_impl->results.clear();
	}

	if (m_impl->nonBlock && work_service != NULL)
	{
		work_service->post(boost::bind(&Handle_Query, (ULONG_PTR)this));
	}
	else
	{
		::SetEvent((HANDLE)m_impl->event);
	}
}

lua_State* CDBQueryEvent::GetLuaState()
{
	return m_impl->m_thread;
}

void CDBQueryEvent::SetValue(mongo::DBClientConnection* connection,
							 const char* ns,
							 mongo::Query* queryObj,
							 int nToReturn,
							 int nToSkip,
							 const mongo::BSONObj* fieldsToReturn,
							 bool nonBlock,
							 int event,
							 lua_State* L)
{
	m_impl->m_thread = L;
	m_impl->connection = connection;
	m_impl->ns = ns;
	m_impl->nToReturn = nToReturn;
	m_impl->nToSkip = nToSkip;
	m_impl->fieldsToReturn = fieldsToReturn;
	m_impl->queryObj = queryObj;
	m_impl->nonBlock = nonBlock;
	m_impl->event = event;
}

CDBQueryEvent::CDBQueryEvent()
{

}

void CDBQueryEvent::Clear()
{
	if (m_impl->queryObj != NULL)
	{
		delete m_impl->queryObj;
	}
	if (m_impl->fieldsToReturn != NULL)
	{
		delete m_impl->fieldsToReturn;
	}
	m_impl->results.clear();
}

//////////////////////////////////////////////////////////

template<>
struct Loki::ImplOf<CDBInsertEvent>
{
	mongo::DBClientConnection* connection;
	const char* ns;
	mongo::BSONObj* obj;
	lua_State* m_thread;
	int writeConcern;
	mongo::BSONObj retInfo;
};

void CDBInsertEvent::Do_Excute()
{
	try
	{
		m_impl->connection->resetError();
		m_impl->connection->insert(m_impl->ns, *m_impl->obj);
		if (m_impl->writeConcern > 0)
		{
			mongo_getLastError(m_impl->connection, m_impl->writeConcern, m_impl->retInfo);
		}
	}
	catch (...)
	{
	}

	if (work_service != NULL)
	{
		work_service->post(boost::bind(&Handle_Insert, (ULONG_PTR)this));
	}
}

lua_State* CDBInsertEvent::GetLuaState()
{
	return m_impl->m_thread;
}

void CDBInsertEvent::SetValue(mongo::DBClientConnection* connection,
							  const char* ns,
							  mongo::BSONObj* obj,
							  int writeConcern,
							  lua_State* L)
{
	m_impl->m_thread = L;
	m_impl->connection = connection;
	m_impl->ns = ns;
	m_impl->obj = obj;
	m_impl->writeConcern = writeConcern;
}

bool CDBInsertEvent::GetResult(mongo::BSONObj& retInfo)
{
	retInfo = m_impl->retInfo;
	return true;
}

CDBInsertEvent::CDBInsertEvent()
{

}

void CDBInsertEvent::Clear()
{
	if (m_impl->obj != NULL)
	{
		delete m_impl->obj;
	}
}

//////////////////////////////////////////////////////////

template<>
struct Loki::ImplOf<CDBUpdateEvent>
{
	mongo::DBClientConnection* connection;
	const char* ns;
	mongo::Query* query;
	mongo::BSONObj* obj;
	lua_State* m_thread;
	bool upsert;
	bool mutil;
	int writeConcern;
	mongo::BSONObj retInfo;
};

void CDBUpdateEvent::Do_Excute()
{
	try
	{
		m_impl->connection->resetError();
		m_impl->connection->update(m_impl->ns, *m_impl->query, *m_impl->obj, m_impl->upsert, m_impl->mutil);
		if (m_impl->writeConcern > 0)
		{
			mongo_getLastError(m_impl->connection, m_impl->writeConcern, m_impl->retInfo);
		}
	}
	catch (...)
	{
	}

	if (work_service != NULL)
	{
		work_service->post(boost::bind(&Handle_Update, (ULONG_PTR)this));
	}
}

lua_State* CDBUpdateEvent::GetLuaState()
{
	return m_impl->m_thread;
}

void CDBUpdateEvent::SetValue(mongo::DBClientConnection* connection,
							  const char* ns,
							  mongo::Query* query,
							  mongo::BSONObj* obj,
							  bool upsert,
							  bool mutil,
							  int writeConcern,
							  lua_State* L)
{
	m_impl->m_thread = L;
	m_impl->connection = connection;
	m_impl->ns = ns;
	m_impl->obj = obj;
	m_impl->query = query;
	m_impl->upsert = upsert;
	m_impl->mutil = mutil;
	m_impl->writeConcern = writeConcern;
}

bool CDBUpdateEvent::GetResult(mongo::BSONObj& retInfo)
{
	retInfo = m_impl->retInfo;
	return true;
}

CDBUpdateEvent::CDBUpdateEvent()
{

}

void CDBUpdateEvent::Clear()
{
	if (m_impl->query != NULL)
	{
		delete m_impl->query;
	}
	if (m_impl->obj != NULL)
	{
		delete m_impl->obj;
	}
}

//////////////////////////////////////////////////////////

template<>
struct Loki::ImplOf<CDBRemoveEvent>
{
	mongo::DBClientConnection* connection;
	const char* ns;
	mongo::Query* query;
	lua_State* m_thread;
	bool justOne;
	int writeConcern;
	mongo::BSONObj retInfo;
};

void CDBRemoveEvent::Do_Excute()
{
	try
	{
		m_impl->connection->resetError();
		m_impl->connection->remove(m_impl->ns, *m_impl->query, m_impl->justOne);
		if (m_impl->writeConcern > 0)
		{
			mongo_getLastError(m_impl->connection, m_impl->writeConcern, m_impl->retInfo);
		}
	}
	catch (...)
	{
	}

	if (work_service != NULL)
	{
		work_service->post(boost::bind(&Handle_Remove, (ULONG_PTR)this));
	}
}

lua_State* CDBRemoveEvent::GetLuaState()
{
	return m_impl->m_thread;
}

void CDBRemoveEvent::SetValue(mongo::DBClientConnection* connection,
							  const char* ns,
							  mongo::Query* query,
							  bool justOne,
							  int writeConcern,
							  lua_State* L)
{
	m_impl->m_thread = L;
	m_impl->connection = connection;
	m_impl->ns = ns;
	m_impl->query = query;
	m_impl->justOne = justOne;
	m_impl->writeConcern = writeConcern;
}

bool CDBRemoveEvent::GetResult(mongo::BSONObj& retInfo)
{
	retInfo = m_impl->retInfo;
	return true;
}

CDBRemoveEvent::CDBRemoveEvent()
{

}

void CDBRemoveEvent::Clear()
{
	if (m_impl->query != NULL)
	{
		delete m_impl->query;
	}
}

/////////////////////////////////////////////////////////////


template<>
struct Loki::ImplOf<CDBCommandEvent>
{
	mongo::DBClientConnection* connection;
	const char* ns;
	mongo::BSONObj* obj;
	mongo::BSONObj retInfo;
	lua_State* m_thread;
	bool bSuccess;
};

void CDBCommandEvent::SetValue(mongo::DBClientConnection* connection,
							   const char* ns,
							   mongo::BSONObj* obj,
							   lua_State* L)
{
	m_impl->m_thread = L;
	m_impl->connection = connection;
	m_impl->ns = ns;
	m_impl->obj = obj;
}

lua_State* CDBCommandEvent::GetLuaState()
{
	return m_impl->m_thread;
}

bool CDBCommandEvent::GetResult(mongo::BSONObj& retInfo)
{
	retInfo = m_impl->retInfo;
	return m_impl->bSuccess;
}

void CDBCommandEvent::Do_Excute()
{
	try
	{
		bool bRet = m_impl->connection->runCommand(m_impl->ns, *m_impl->obj, m_impl->retInfo);
		if (bRet)
		{
			m_impl->bSuccess = true;
		}
	}
	catch (...)
	{
		m_impl->bSuccess = false;
	}

	if (work_service != NULL)
	{
		work_service->post(boost::bind(&Handle_Command, (ULONG_PTR)this));
	}
}

CDBCommandEvent::CDBCommandEvent()
{

}

void CDBCommandEvent::Clear()
{
	if (m_impl->obj != NULL)
	{
		delete m_impl->obj;
	}
}

//////////////////////////////////////////////////////
template<>
struct Loki::ImplOf<CDBCountEvent>
{
	mongo::DBClientConnection* connection;
	const char* ns;
	mongo::BSONObj* obj;
	int nToReturn;
	int nToSkip;
	lua_State* m_thread;
	unsigned long long retCount;
};

int CDBCountEvent::GetResult()
{
	return (int)m_impl->retCount;
}

void CDBCountEvent::Do_Excute()
{
	try
	{
		m_impl->retCount = m_impl->connection->count(
			m_impl->ns, *m_impl->obj, 0, m_impl->nToReturn, m_impl->nToSkip);
	}
	catch (...)
	{
		m_impl->retCount = 0;
	}

	if (work_service != NULL)
	{
		work_service->post(boost::bind(&Handle_Count, (ULONG_PTR)this));
	}
}

lua_State* CDBCountEvent::GetLuaState()
{
	return m_impl->m_thread;
}

void CDBCountEvent::SetValue(mongo::DBClientConnection* connection,
							 const char* ns,
							 mongo::BSONObj* obj,
							 int nToReturn,
							 int nToSkip,
							 lua_State* L)
{
	m_impl->m_thread = L;
	m_impl->connection = connection;
	m_impl->ns = ns;
	m_impl->nToReturn = nToReturn;
	m_impl->nToSkip = nToSkip;
	m_impl->obj = obj;
}

CDBCountEvent::CDBCountEvent()
{

}

void CDBCountEvent::Clear()
{
	if (m_impl->obj != NULL)
	{
		delete m_impl->obj;
	}
}
