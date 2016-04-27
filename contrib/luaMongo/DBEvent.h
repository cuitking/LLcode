#pragma once

#include <loki/Pimpl.h>
#include <lua/lua.hpp>
#include <vector>
#include <map>

namespace mongo
{
	class DBClientCursor;
	class DBClientConnection;
	class Query;
	class BSONObj;
}

class CDBEvent
{
public:
	virtual ~CDBEvent() = 0;
	virtual void Do_Excute() = 0;
	virtual lua_State* GetLuaState() = 0;
	virtual void Clear() = 0;
	virtual bool GetResult(mongo::BSONObj& retInfo);
};

class CDBQueryEvent : public CDBEvent
{
public:
	CDBQueryEvent();
public:
	void Do_Excute();
	std::vector<mongo::BSONObj>& GetResult();
	lua_State* GetLuaState();
	void Clear();
	void SetValue(
		mongo::DBClientConnection* connection,
		const char* ns,
		mongo::Query* queryObj,
		int nToReturn,
		int nToSkip,
		const mongo::BSONObj* fieldsToReturn,
		bool nonBlock,
		int event,
		lua_State* L
	);

private:
	Loki::PimplOf<CDBQueryEvent>::Type m_impl;
};

class CDBInsertEvent : public CDBEvent
{
public:
	CDBInsertEvent();
public:
	void Do_Excute();
	lua_State* GetLuaState();
	bool GetResult(mongo::BSONObj& retInfo);
	void Clear();
	void SetValue(
		mongo::DBClientConnection* connection,
		const char* ns,
		mongo::BSONObj* obj,
		int writeConcern,
		lua_State* L
		);

private:
	Loki::PimplOf<CDBInsertEvent>::Type m_impl;
};

class CDBUpdateEvent : public CDBEvent
{
public:
	CDBUpdateEvent();
public:
	void Do_Excute();
	lua_State* GetLuaState();
	bool GetResult(mongo::BSONObj& retInfo);
	void Clear();
	void SetValue(
		mongo::DBClientConnection* connection,
		const char* ns,
		mongo::Query* query,
		mongo::BSONObj* obj,
		bool upsert,
		bool mutil,
		int writeConcern,
		lua_State* L
		);

private:
	Loki::PimplOf<CDBUpdateEvent>::Type m_impl;
};

class CDBRemoveEvent : public CDBEvent
{
public:
	CDBRemoveEvent();
public:
	void Do_Excute();
	lua_State* GetLuaState();
	bool GetResult(mongo::BSONObj& retInfo);
	void Clear();
	void SetValue(
		mongo::DBClientConnection* connection,
		const char* ns,
		mongo::Query* query,
		bool justOne,
		int writeConcern,
		lua_State* L
		);

private:
	Loki::PimplOf<CDBRemoveEvent>::Type m_impl;
};


class CDBCommandEvent : public CDBEvent
{
public:
	CDBCommandEvent();
public:
	void Do_Excute();
	bool GetResult(mongo::BSONObj& retInfo);
	lua_State* GetLuaState();
	void Clear();
	void SetValue(
		mongo::DBClientConnection* connection,
		const char* ns,
		mongo::BSONObj* obj,
		lua_State* L
		);

private:
	Loki::PimplOf<CDBCommandEvent>::Type m_impl;
};
class CDBCountEvent : public CDBEvent
{
public:
	CDBCountEvent();
public:
	void Do_Excute();
	int GetResult();
	lua_State* GetLuaState();
	void Clear();
	void SetValue(
		mongo::DBClientConnection* connection,
		const char* ns,
		mongo::BSONObj* obj,
		int nToReturn,
		int nToSkip,
		lua_State* L
		);

private:
	Loki::PimplOf<CDBCountEvent>::Type m_impl;
};
