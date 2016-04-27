#ifndef PLAYER_DETAIL_H
#define PLAYER_DETAIL_H
#include "Types.h"
#include "Observer.h"
#include <time.h>
namespace scgl
{
	class CPlayerContainer;

	class CPlayerDetail
	{
	public:
		CPlayerDetail(scgl::PlayerIDType playerID, scgl::Guid playerGuid);
		virtual ~CPlayerDetail();
		bool isActive();
		bool isXPInited();
		void setDead();
		void setXPInited();
		void SetContainer(CPlayerContainer* container);

		void SetGateGuid(scgl::Guid guid);
		void SetGameServerGuid(scgl::Guid guid);
		void SetPlayerID(scgl::PlayerIDType playerID);
		void SetPlayerGuid(scgl::Guid playerGuid);
		void SetRoomID(int roomID);
		void SetLastActiveTime(long currentTime);
		void SetClientLastActiveTime(long currentTime);

		scgl::PlayerIDType GetPlayerID() const;
		scgl::Guid GetPlayerGuid() const;
		scgl::Guid GetGateGuid() const;
		scgl::Guid GetGameServerGuid() const;
		int GetRoomID() const;
		long GetLastActiveTime() const;
		long GetClientLastActiveTime() const;
		
	public:
		scgl::PlayerIDType	 m_playerID;
		scgl::Guid			 m_playerGuid;

	private:
		scgl::Guid			 m_gateGuid;
		scgl::Guid			 m_gameServerGuid;
		CPlayerContainer*    m_container;
		int			m_roomID;
		bool		actived;
		long		clientLastActiveTime;
		long		lastActiveTime;
	};
}
#endif

