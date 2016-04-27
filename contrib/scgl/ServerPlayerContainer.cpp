#include "ServerPlayerContainer.h"
#include "PlayerDetail.h"
#include <boost/foreach.hpp>
#include <algorithm>


namespace scgl
{
	void IServerPlayerContainer::DeletePlayer(Guid serverGuid, scgl::PlayerIDType playerID)
	{
		ServerPlayerMap::iterator it = m_serverPlayers.find(serverGuid);
		if (it != m_serverPlayers.end())
		{
			PlayerIDMap& players = it->second;
			players.erase(playerID);
			if (players.empty())
			{
				m_serverPlayers.erase(it);
			}
		}
	}

	void IServerPlayerContainer::AddPlayer(Guid serverGuid, scgl::PlayerIDType playerID, Guid playerGuid)
	{
		ServerPlayerMap::iterator it = m_serverPlayers.find(serverGuid);
		if (it == m_serverPlayers.end())
		{
			PlayerIDMap players;
			players.insert(std::make_pair(playerID, playerGuid));
			m_serverPlayers.insert(std::make_pair(serverGuid, players));
		}
		else
		{
			it->second.insert(std::make_pair(playerID, playerGuid));
		}
	}

	const PlayerIDMap* IServerPlayerContainer::GetPlayers(Guid serverGuid) const
	{
		ServerPlayerMap::const_iterator it = m_serverPlayers.find(serverGuid);
		if (it != m_serverPlayers.end())
		{
			return &it->second;
		}
		return NULL;
	}

	void CLobbyPlayerContainer::Update(scgl::CPlayerDetail* sender, scgl::Guid oldGuid)
	{
		scgl::PlayerIDType playerID = sender->GetPlayerID();
		scgl::Guid newGuid = sender->GetGameServerGuid();
		if (oldGuid != 0)
		{
			DeletePlayer(oldGuid, playerID);
		}
		if (newGuid != 0)
		{
			AddPlayer(newGuid, playerID, sender->GetPlayerGuid());
		}
	}

	void CRoomPlayerContainer::Update(scgl::CPlayerDetail* sender, scgl::Guid oldGuid)
	{
		scgl::PlayerIDType playerID = sender->GetPlayerID();
		scgl::Guid newGuid = sender->GetRoomID();
		if (oldGuid != 0)
		{
			DeletePlayer(oldGuid, playerID);
		}
		if (newGuid != 0)
		{
			AddPlayer(newGuid, playerID, sender->GetPlayerGuid());
		}
	}
}
