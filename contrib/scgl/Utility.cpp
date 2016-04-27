#include "Utility.h"
#include "Win32.h"
#include "Types.h"
#include "Log.h"
#include "StringAlgorithm.h"
#include <boost/thread.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>

namespace scgl
{
	unsigned int Generate32BitGuid(void)
	{
		boost::uuids::random_generator gen;
		unsigned int g = 0;
		while (g <= 100000000)
		{
			g = boost::uuids::hash_value(gen());
		}
		return g;
	}
	void ShowSystemErrorMessage()
	{
		TCHAR szMessageBuffer[4096];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
					  NULL, ::GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					  szMessageBuffer, sizeof(szMessageBuffer), NULL);
		::MessageBox(NULL, szMessageBuffer, NULL, MB_OK);
	}

	const char* GenerateUuid(const char* prefix)
	{
		static boost::mutex m;
		static std::string s;

		boost::mutex::scoped_lock lock(m);

		s.clear();
		if (prefix != NULL)
		{
			s += prefix;
		}

		boost::uuids::random_generator gen;
		s += boost::lexical_cast<std::string>(gen());
		return s.c_str();
	}
}
