#include "StringAlgorithm.h"

namespace scgl
{
	inline char HexToChar(char c)
	{
		if ('0' <= c && c <= '9')
		{
			return c - '0';
		}
		else if ('a' <= c && c <= 'f')
		{
			return c + 10 - 'a';
		}
		else if ('A' <= c && c <= 'F')
		{
			return c + 10 - 'A';
		}

		return -1;
	}

	size_t HexToBinrary( const char* hex, size_t length, char* binrary, size_t binrary_cap )
	{
		if (length % 2 != 0 || binrary_cap < length / 2)
		{
			return 0;
		}

		size_t n = 0;
		for (size_t i = 0; i < length; i += 2, ++n)
		{
			char high = HexToChar(hex[i]);
			if (high < 0)
			{
				return 0;
			}

			char low = HexToChar(hex[i + 1]);
			if (low < 0)
			{
				return 0;
			}

			binrary[n] = high << 4 | low;
		}

		return n;
	}

	size_t BinraryToHex( const char* binrary, size_t binrary_cap, char* hex, size_t length )
	{
		if (length < binrary_cap * 2)
		{
			return 0;
		}

		size_t n = 0;
		for (size_t i = 0; i < binrary_cap; ++i, n += 2)
		{
			char high = (binrary[i] & 0xF0) >> 4;
			if (high >= 0 && high <= 9)
			{
				hex[n] = high + '0';
			}
			else
			{
				hex[n] = high + 'A' - 10;
			}
			
			char low = binrary[i] & 0x0F;
			if (low >= 0 && low <= 9)
			{
				hex[n+1] = low + '0';
			}
			else
			{
				hex[n+1] = low + 'A' - 10;
			}
		}

		return n;
	}


}
