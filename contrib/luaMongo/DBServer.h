#pragma once

#include "common.h"
#include <scgl/SingletonHolder.h>

class CDBEvent;

class SDBServer
{
	DECLARE_SINGLETON(SDBServer)
public:
	SDBServer(void);
	virtual ~SDBServer(void);

public:
	bool AsyncProcess(void* connection, CDBEvent* pEvent);
	void AddConnection(void* connection);
	void UnInit();
#ifdef _DEBUG
	void SetSleepTime(int sleepTime);
	int GetSleepTime();
#endif

private:
	struct Impl;

private:
	Impl* m_impl;
};
