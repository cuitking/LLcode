#ifndef SCGL_CONNECTION_MANAGER_H
#define SCGL_CONNECTION_MANAGER_H

#include "Types.h"

namespace scgl
{
	class CConnection;

	class IConnectionManager
	{
	public:
		virtual int InsertConnection(ConnectionPtr connection) = 0;
		virtual const ConnectionPtr FindConnection(scgl::Guid guid) const = 0;
	};
}
#endif