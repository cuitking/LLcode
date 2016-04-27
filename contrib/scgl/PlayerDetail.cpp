#include "PlayerDetail.h"
#include "PlayerContainer.h"
#include <boost/foreach.hpp>
#ifndef _WIN32 
#include <sys/time.h> 
#endif

namespace scgl
{
	CPlayerDetail::CPlayerDetail(scgl::PlayerIDType playerID, scgl::Guid playerGuid)
		: m_playerID(playerID)
		, m_playerGuid(playerGuid)
		, m_gateGuid(0)
		, m_gameServerGuid(0)
		, m_roomID(0)
		, m_container(NULL)
		, actived(true)
	{
#ifdef _WIN32
		lastActiveTime = time(NULL);
#else
		struct timeval tv; 
		gettimeofday(&tv, NULL);
		lastActiveTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif			
	}

	CPlayerDetail::~CPlayerDetail()
	{
		if (m_roomID != 0)
		{
			SetRoomID(0);
		}
		if (m_gameServerGuid != 0)
		{
			SetGameServerGuid(0);
		}
	}

	void CPlayerDetail::SetGateGuid(Guid guid)
	{
		m_gateGuid = guid;
	}

	void CPlayerDetail::SetGameServerGuid(Guid guid)
	{
		if (m_gameServerGuid != guid)
		{
			scgl::Guid oldGuid = m_gameServerGuid;
			m_gameServerGuid = guid;
			if (m_container != NULL)
			{
				m_container->Notify(this, Notify_GameServer, oldGuid);
			}
		}
	}

	void CPlayerDetail::SetRoomID(int roomID)
	{
		if (m_roomID != roomID)
		{
			scgl::Guid oldGuid = m_roomID;
			m_roomID = roomID;
			if (m_container != NULL)
			{
				m_container->Notify(this, Notify_Room, oldGuid);
			}
		}
	}

	Guid CPlayerDetail::GetGateGuid() const
	{
		return m_gateGuid;
	}

	Guid CPlayerDetail::GetGameServerGuid() const
	{
		return m_gameServerGuid;
	}

	scgl::PlayerIDType CPlayerDetail::GetPlayerID() const
	{
		return m_playerID;
	}

	Guid CPlayerDetail::GetPlayerGuid() const
	{
		return m_playerGuid;
	}

	int CPlayerDetail::GetRoomID() const
	{
		return m_roomID;
	}

	long CPlayerDetail::GetLastActiveTime() const
	{
		return lastActiveTime;
	}

	long CPlayerDetail::GetClientLastActiveTime() const
	{
		return clientLastActiveTime;
	}

	void CPlayerDetail::SetPlayerID(scgl::PlayerIDType playerID)
	{
		m_playerID = playerID;
	}

	void CPlayerDetail::SetPlayerGuid(scgl::Guid playerGuid)
	{
		m_playerGuid = playerGuid;
	}

	void CPlayerDetail::SetLastActiveTime(long currentTime)
	{
		lastActiveTime = currentTime;
	}

	void CPlayerDetail::SetClientLastActiveTime(long currentTime)
	{
		clientLastActiveTime = currentTime;
	}

	void CPlayerDetail::SetContainer(CPlayerContainer* container)
	{
		m_container = container;
	}

	void CPlayerDetail::setDead()
	{
		actived = false;
	}

	bool CPlayerDetail::isActive()
	{
		return actived;
	}

}