#include "Behavior.h"
#include "Types.h"
#include "Reactor.h"
#include "Trigger.h"

std::set<int> scgl::IBehavior<scgl::CReactor, scgl::CTrigger, int, const scgl::Header, const scgl::Message*>::transactionSet;

