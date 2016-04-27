#pragma once

#include <scgl/Types.h>
#include <scgl/Behavior.h>
#include <scgl/Reactor.h>
#include <scgl/Trigger.h>

namespace pb
{
	class Header;
}

class CPlayerState : public scgl::IBehavior<scgl::CReactor, scgl::CTrigger, int, const scgl::Header, const scgl::Message*>
{
public:
	CPlayerState(void);
	virtual ~CPlayerState(void);
};
