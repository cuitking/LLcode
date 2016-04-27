#include "Net.h"
#include "../pb/header.pb.h"

namespace scgl
{
	INet::INet(void)
	{

	}

	INet::~INet(void)
	{

	}

	void DumpData( const char* data, int len, std::ostringstream& oss )
	{
		oss << std::setiosflags(std::ios::uppercase) << std::hex;
		const int width = 16;
		for (int i=1;i<=len;i++)
		{
			oss << "0x" << std::setw(2) << std::setfill('0') <<(unsigned int)(data[i] & 0xFF);
			if (i%width == 0)
			{
				oss << "\n";
			}
			else if(i%width < width)
			{
				oss << ",";
			}
		}
	}

}