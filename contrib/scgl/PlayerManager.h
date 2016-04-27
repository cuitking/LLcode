#pragma once
#include "PlayerDetail.h"
#include "PlayerContainer.h"
#include "ServerPlayerContainer.h"
#include "SingletonHolder.h"
namespace scgl
{
	class SPlayerManager
	{
	public:
		void SetCreator(PlayerCreator playerCreator);
		void SetDeletor(PlayerDeletor playerDeletor);
		CPlayerDetail* Add(scgl::PlayerIDType playerID, scgl::Guid playerGuid);
		bool Add(CPlayerDetail* player);
		CPlayerDetail* GetByPlayerID(scgl::PlayerIDType playerID);
		CPlayerDetail* GetByPlayerGuid(scgl::Guid playerGuid);
		bool	 Delete(scgl::PlayerIDType playerID);
		bool	 Remove(CPlayerDetail* player);
		size_t	 Size();

		const PlayerIDMap* GetPlayersInGameServer(scgl::Guid gameServerGuid) const;
		const PlayerIDMap* GetPlayersInRoom(unsigned short roomID) const;
		void Update(long currentTime, std::vector<scgl::Guid>& deadPlayers);

	private:
		SPlayerManager();
		~SPlayerManager();

	private:
		CPlayerContainer*		m_players;
		CLobbyPlayerContainer*	m_gameServer;
		CRoomPlayerContainer*	m_room;

		DECLARE_SINGLETON(SPlayerManager);
	};

}