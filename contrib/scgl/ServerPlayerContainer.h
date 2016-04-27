#ifndef SERVER_PLAYER_CONTAINER_H
#define SERVER_PLAYER_CONTAINER_H

#include "Types.h"
#include "Observer.h"
#include <map>

namespace scgl
{
	typedef std::map<scgl::PlayerIDType, Guid> PlayerIDMap;
	typedef std::map<Guid, PlayerIDMap>  ServerPlayerMap;

	class IServerPlayerContainer : public IObserver
	{
	public:
		virtual ~IServerPlayerContainer() {};
	public:
		void DeletePlayer(Guid serverGuid, scgl::PlayerIDType playerID);
		void AddPlayer(Guid serverGuid, scgl::PlayerIDType playerID, Guid playerGuid);
		const PlayerIDMap* GetPlayers(Guid serverGuid) const;
	private:
		ServerPlayerMap m_serverPlayers;
	};

	class CLobbyPlayerContainer : public IServerPlayerContainer
	{
	public:
		virtual void Update(scgl::CPlayerDetail* sender, scgl::Guid oldGuid = 0);
	};

	class CRoomPlayerContainer : public IServerPlayerContainer
	{
	public:
		virtual void Update(scgl::CPlayerDetail* sender, scgl::Guid oldGuid = 0);
	};

}
#endif