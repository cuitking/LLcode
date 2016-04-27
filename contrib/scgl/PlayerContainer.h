#ifndef PLAYER_CONTAINER_H
#define PLAYER_CONTAINER_H

#include "Types.h"
#include "Observer.h"
#include <boost/function.hpp>
#include <map>
#include <vector>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include "PlayerDetail.h"
namespace scgl
{
	typedef boost::function<CPlayerDetail* (scgl::PlayerIDType playerID, scgl::Guid playerGuid)>	PlayerCreator;
	typedef boost::function<void (CPlayerDetail* player)>	PlayerDeletor;
	typedef std::map<int, IObserver*> ObserverMap;

	struct PlayerDetailWrapper
	{
		scgl::PlayerIDType playerID;
		scgl::Guid playerGuid;
		CPlayerDetail* player;
		PlayerDetailWrapper()
			: playerID(0)
			, playerGuid(0)
			, player(NULL)
		{

		}
		PlayerDetailWrapper(scgl::PlayerIDType _playerID, scgl::Guid _playerGuid, CPlayerDetail* _player)
			: playerID(_playerID)
			, playerGuid(_playerGuid)
			, player(_player)
		{

		}
	};

	namespace bm = boost::multi_index;
	struct PLAYERID {};
	struct PLAYERGUID {};
	typedef boost::multi_index_container <
		PlayerDetailWrapper,
		bm::indexed_by <
			bm::hashed_non_unique <
				bm::tag<PLAYERID>,
				bm::member<PlayerDetailWrapper, scgl::PlayerIDType, &PlayerDetailWrapper::playerID>
			> ,
			bm::hashed_unique <
				bm::tag<PLAYERGUID>,
				bm::member<PlayerDetailWrapper, scgl::Guid, &PlayerDetailWrapper::playerGuid>
			>
		>
	> PlayerMap;

	class CPlayerContainer
	{
	public:
		CPlayerContainer();
		virtual ~CPlayerContainer();
	public:
		void SetCreator(PlayerCreator playerCreator);
		void SetDeletor(PlayerDeletor playerDeletor);
		CPlayerDetail* Add(scgl::PlayerIDType playerID, scgl::Guid playerGuid);
		bool Add(CPlayerDetail* player);
		CPlayerDetail* GetByPlayerID(scgl::PlayerIDType playerID);
		CPlayerDetail* GetByPlayerGuid(scgl::Guid playerGuid);
		bool	 Delete(scgl::PlayerIDType playerID);
		bool	 Remove(CPlayerDetail* player);
		void	 SetPlayerObserver(int notifyType, IObserver* observer);
		void     ClearObservers();
		size_t	 Size();
	public:
		void  Notify(CPlayerDetail* player, int notifyType, scgl::Guid oldGuid = 0);
		void Update(long currentTime, std::vector<scgl::Guid>& deadPlayers);
	private:
		PlayerMap		m_players;
		ObserverMap		m_observers;
		PlayerCreator	m_creator;
		PlayerDeletor   m_deletor;
		int				playerNormalTimerOut;
		int				playerBattleTimerOut;
	};
}
#endif