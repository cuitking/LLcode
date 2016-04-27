#pragma once

#include "PropertyMap.h"
#include <deque>

namespace scgl
{

	struct Parameter
	{
		int			ID;
		CPropertyMap	parameter;

		Parameter();
		Parameter(int ID_);
	};

	enum EVENT_ID
	{
		UPDATE	= 1,
		CREATE	= 2,
	};

	typedef	std::deque<Parameter>							EventDeque;

}
