#include "PlayerManager.h"
#include <boost/foreach.hpp>
#include <algorithm>
namespace scgl
{
	DEFINITION_SINGLETON(SPlayerManager);

	CPlayerDetail* SPlayerManager::Add(scgl::PlayerIDType playerID, Guid playerGuid)
	{
		return m_players->Add(playerID, playerGuid);
	}

	bool SPlayerManager::Add(CPlayerDetail* player)
	{
		return m_players->Add(player);
	}

	CPlayerDetail* SPlayerManager::GetByPlayerID(scgl::PlayerIDType playerID)
	{
		return m_players->GetByPlayerID(playerID);
	}

	bool SPlayerManager::Delete(scgl::PlayerIDType playerID)
	{
		return m_players->Delete(playerID);
	}

	bool SPlayerManager::Remove(CPlayerDetail* player)
	{
		return m_players->Remove(player);
	}

	size_t SPlayerManager::Size()
	{
		return m_players->Size();
	}

	SPlayerManager::SPlayerManager()
		: m_players(new CPlayerContainer)
		, m_gameServer(new CLobbyPlayerContainer)
		, m_room(new CRoomPlayerContainer)
	{
		m_players->SetPlayerObserver(Notify_GameServer, m_gameServer);
		m_players->SetPlayerObserver(Notify_Room, m_room);
	}

	SPlayerManager::~SPlayerManager()
	{
		delete m_gameServer;
		delete m_room;
		m_players->ClearObservers();
		delete m_players;
	}

	const PlayerIDMap* SPlayerManager::GetPlayersInGameServer(Guid gameServerGuid) const
	{
		return m_gameServer->GetPlayers(gameServerGuid);
	}

	CPlayerDetail* SPlayerManager::GetByPlayerGuid(Guid playerGuid)
	{
		return m_players->GetByPlayerGuid(playerGuid);
	}

	void SPlayerManager::SetCreator(PlayerCreator playerCreator)
	{
		m_players->SetCreator(playerCreator);
	}

	const PlayerIDMap* SPlayerManager::GetPlayersInRoom(unsigned short roomID) const
	{
		return m_room->GetPlayers(roomID);
	}

	void SPlayerManager::SetDeletor(PlayerDeletor playerDeletor)
	{
		m_players->SetDeletor(playerDeletor);
	}

	void SPlayerManager::Update( long currentTime, std::vector<scgl::Guid>& deadPlayers )
	{
		m_players->Update(currentTime, deadPlayers);
	}

}