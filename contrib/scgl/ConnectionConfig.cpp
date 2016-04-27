#include "ConnectionConfig.h"
#include <memory.h>

namespace scgl
{
	SConnectionConfig::SConnectionConfig( pb::EConnectionType _type, bool _tcp /*= true*/ )
	{
		memset(this, 0, sizeof(*this));
		type = _type;
		tcp = _tcp;
		autoReConnect = false;
	}

}
