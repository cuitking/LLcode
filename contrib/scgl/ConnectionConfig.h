#ifndef SCGL_CONNECTION_CONFIG_H
#define SCGL_CONNECTION_CONFIG_H

#include "Types.h"

namespace pb
{
	enum EConnectionType;
}

namespace scgl
{
	// 要求为POD类型
	struct SConnectionConfig
	{
		enum { MAX_IP_CAPACITY = 32 };

		pb::EConnectionType	type;
		unsigned short		port;
		unsigned short		maxConnectionAmount;
		bool				tcp;
		bool				listener;
		int					readBufferCapacity;
		int					writeBufferCapacity;
		char				IP[MAX_IP_CAPACITY];
		bool				autoReConnect;

		explicit SConnectionConfig(pb::EConnectionType _type, bool _tcp = true);
	};

	typedef std::vector<SConnectionConfig>	ConnectionConfigVector;

}

#endif
