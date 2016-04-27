#pragma once

#include <scgl/PlayerManager.h>
#include <scgl/PlayerState.h>

class CPlayer : public scgl::CPlayerDetail, public CPlayerState
{
public:
	explicit CPlayer(scgl::PlayerIDType playerID, scgl::Guid playerGuid);
	virtual ~CPlayer(void);

};

//创建玩家
CPlayer* CreatePlayer(scgl::PlayerIDType playerID, scgl::Guid playerGuid);
//删除玩家
void DeletePlayer(scgl::CPlayerDetail* player);

