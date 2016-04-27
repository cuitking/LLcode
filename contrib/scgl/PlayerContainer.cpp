#include "PlayerContainer.h"
#include "PlayerManager.h"
#include <boost/foreach.hpp>
#include <scgl/Log.h>
#include <scgl/Player.h>
#include <boost/cast.hpp>
#include <scgl/Variant.h>
#include <scgl/Packet.h>

namespace scgl
{
	CPlayerDetail* CPlayerContainer::Add(scgl::PlayerIDType playerID, Guid playerGuid)
	{
		const PlayerMap::index<PLAYERID>::type& playerIDMap = m_players.get<PLAYERID>();
		PlayerMap::index<PLAYERID>::type::iterator it = playerIDMap.find(playerID);
		if (it != playerIDMap.end())
		{
			const PlayerDetailWrapper& playerDetail = *it;
			return playerDetail.player;
		}
		CPlayerDetail* player = m_creator(playerID, playerGuid);
		assert(player != NULL);
		player->SetContainer(this);
		if (!m_players.insert(PlayerDetailWrapper(playerID, playerGuid, player)).second)
		{
			LOG_WARN("Net", "CPlayerContainer::Add error!!!" << 
				" playerID:" << playerID << 
				" playerGuid:" << playerGuid
			);
			return NULL;
		}
		return player;
	}

	bool CPlayerContainer::Add(CPlayerDetail* player)
	{
		if (player == NULL)
		{
			return false;
		}
		player->SetContainer(this);
		if (!m_players.insert(PlayerDetailWrapper(player->m_playerID, player->m_playerGuid, player)).second)
		{
			LOG_WARN("Net", "CPlayerContainer::Add 2 error!!!" << 
				" playerID:" << player->m_playerID << 
				" playerGuid:" << player->m_playerGuid
			);
			return false;
		}
		return true;
	}

	CPlayerDetail* CPlayerContainer::GetByPlayerID(scgl::PlayerIDType playerID)
	{
		if (playerID == 0)
		{
			return NULL;
		}
		const PlayerMap::index<PLAYERID>::type& playerIDMap = m_players.get<PLAYERID>();
		PlayerMap::index<PLAYERID>::type::iterator it = playerIDMap.find(playerID);
		if (it != playerIDMap.end())
		{
			const PlayerDetailWrapper& playerDetail = *it;
			return playerDetail.player;
		}
		return NULL;
	}

	CPlayerDetail* CPlayerContainer::GetByPlayerGuid(Guid playerGuid)
	{
		const PlayerMap::index<PLAYERGUID>::type& playerGuidMap = m_players.get<PLAYERGUID>();
		PlayerMap::index<PLAYERGUID>::type::iterator it = playerGuidMap.find(playerGuid);
		if (it != playerGuidMap.end())
		{
			const PlayerDetailWrapper& playerDetail = *it;
			return playerDetail.player;
		}
		return NULL;
	}

	bool CPlayerContainer::Delete(scgl::PlayerIDType playerID)
	{
		if (playerID == 0)
		{
			return false;
		}
		PlayerMap::index<PLAYERID>::type& playerIDMap = m_players.get<PLAYERID>();
		PlayerMap::index<PLAYERID>::type::iterator it = playerIDMap.find(playerID);
		if (it != playerIDMap.end())
		{
			const PlayerDetailWrapper& playerDetail = *it;
			m_deletor(playerDetail.player);
			playerIDMap.erase(it);
			return true;
		}
		return false;
	}

	bool CPlayerContainer::Remove(CPlayerDetail* player)
	{
		if (player == NULL)
		{
			return false;
		}
		PlayerMap::index<PLAYERGUID>::type& playerIDMap = m_players.get<PLAYERGUID>();
		PlayerMap::index<PLAYERGUID>::type::iterator it = playerIDMap.find(player->m_playerGuid);
		if (it != playerIDMap.end())
		{
			playerIDMap.erase(it);
			return true;
		}
		return false;
	}

	size_t CPlayerContainer::Size()
	{
		return m_players.size();
	}

	void CPlayerContainer::SetPlayerObserver(int notifyType, IObserver* observer)
	{
		m_observers.insert(std::make_pair(notifyType, observer));
	}

	void CPlayerContainer::ClearObservers()
	{
		m_observers.clear();
	}

	CPlayerContainer::~CPlayerContainer()
	{
		for (PlayerMap::iterator iter = m_players.begin(); iter != m_players.end(); ++iter)
		{
			const PlayerDetailWrapper& playerDetail = *iter;
			m_deletor(playerDetail.player);
		}
		m_players.clear();
	}

	void CPlayerContainer::SetCreator(PlayerCreator playerCreator)
	{
		m_creator = playerCreator;
	}

	void CPlayerContainer::SetDeletor(PlayerDeletor playerDeletor)
	{
		m_deletor = playerDeletor;
	}

	void CPlayerContainer::Notify(CPlayerDetail* player, int notifyType, scgl::Guid oldGuid)
	{
		if (notifyType == Notify_All)
		{
			BOOST_FOREACH(const ObserverMap::value_type & v, m_observers)
			{
				switch (v.first)
				{
				case Notify_GameServer:
					v.second->Update(player, player->GetGameServerGuid());
					break;
				case Notify_Room:
					v.second->Update(player, player->GetRoomID());
					break;
				}
			}
		}
		else
		{
			BOOST_FOREACH(const ObserverMap::value_type & v, m_observers)
			{
				if (v.first == notifyType)
				{
					v.second->Update(player, oldGuid);
					return;
				}
			}
		}
	}

	void CPlayerContainer::Update(long currentTime, std::vector<scgl::Guid>& deadPlayers)
	{
		for (PlayerMap::iterator iter = m_players.begin(); iter != m_players.end(); ++iter)
		{
			const PlayerDetailWrapper& playerDetail = *iter;
			CPlayer* player = boost::polymorphic_cast<CPlayer*>(playerDetail.player);
			if (player != NULL)
			{
				bool isDead = false;
				int state = 0;
				long lastActiveTime = player->GetLastActiveTime();
				if (strcmp(typeid(*player->GetCurrentBehavior()).name(), "class CPlayerStateBattle") == 0)
				{	
					if (currentTime - lastActiveTime >= playerBattleTimerOut)
					{
						isDead = true;
						state = 0;
					}
				}
				else
				{
					if (currentTime - lastActiveTime >= playerNormalTimerOut)
					{
						isDead = true;
						state = 1;
					}
				}
				if (isDead)
				{
					//超过时间没有活动的玩家，认为是已经死亡的连接
					scgl::Guid playerGuid = playerDetail.playerGuid;
					deadPlayers.push_back(playerGuid);
					LOG_INFO("Net", "PlayerTimeOut Add guid:" << playerGuid
						<< ",state:" << state
						<< ",currentTime:" << currentTime 
						<< ",lastActiveTime:" << lastActiveTime
					);
				}
			}
		}
	}

	CPlayerContainer::CPlayerContainer()
	{
		playerNormalTimerOut = 300;
		playerBattleTimerOut = 30;
		const scgl::CVariant& playerNormalTimerOutValue = scgl::GetGlobalParam("playerNormalTimerOut");
		if (playerNormalTimerOutValue != scgl::CVariant::NIL)
		{
			playerNormalTimerOut = playerNormalTimerOutValue.GetNumber<int>();
		}
		const scgl::CVariant& playerBattleTimerOutValue = scgl::GetGlobalParam("playerBattleTimerOut");
		if (playerBattleTimerOutValue != scgl::CVariant::NIL)
		{
			playerBattleTimerOut = playerBattleTimerOutValue.GetNumber<int>();
		}
	}

}
