#ifndef SCGL_STRING_ALGORITHM_H
#define SCGL_STRING_ALGORITHM_H

#include <tchar.h>
#include "tiostream"
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/call_traits.hpp>

namespace scgl
{

	/**
	 * 将字符串转换为任意类型
	 */
	template <typename T>
	inline T FromString(const std::string& s)
	{
		return FromString<T>(s.c_str());
	}

	template <>
	inline std::string FromString(const std::string& s)
	{
		return s;
	}

	template <typename T>
	inline T FromString(const TCHAR* s)
	{
		return boost::lexical_cast<T>(s);
	}

	template <typename T>
	inline T FromStringToEnum(const TCHAR* s)
	{
		return static_cast<T>(FromString<int>(s));
	}

	template <>
	inline long long FromString<long long>(const TCHAR* s)
	{
		return _ttoi64(s);
	}

	template <>
	inline int FromString<int>(const TCHAR* s)
	{
		return _ttoi(s);
	}

	template <>
	inline unsigned char FromString<unsigned char>(const TCHAR* s)
	{
		return static_cast<unsigned char>(FromString<int>(s));
	}

	template <>
	inline bool FromString<bool>(const TCHAR* s)
	{
		return FromString<long long>(s) != 0;
	}

	template <>
	inline double FromString<double>(const TCHAR* s)
	{
		return _tstof(s);
	}

	template <>
	inline float FromString<float>(const TCHAR* s)
	{
		return static_cast<float>(FromString<double>(s));
	}

	template <>
	inline TCHAR FromString<TCHAR>(const TCHAR* s)
	{
		if (s != NULL)
		{
			return s[0];
		}
		return '\0';
	}
	/**
	 * 将任意类型转换为字符串
	 */
	template <typename T>
	inline tstring ToString(const T t)
	{
		static tostringstream os;
		os.str(_T(""));
		os << t;
		return os.str();
	}

	template <>
	inline tstring ToString<__int64>(__int64 number)
	{
		TCHAR strBuffer[65];
		if (_i64tot_s(number, strBuffer, sizeof(strBuffer), 10) == 0)
		{
			return strBuffer;
		}
		else
		{
			return _T("");
		}
	}

	template <>
	inline tstring ToString<short>(short number)
	{
		return ToString<__int64>(number);
	}

	template <>
	inline tstring ToString<unsigned short>(unsigned short number)
	{
		return ToString<__int64>(number);
	}

	template <>
	inline tstring ToString<int>(int number)
	{
		return ToString<__int64>(number);
	}

	template <>
	inline tstring ToString<unsigned int>(unsigned int number)
	{
		return ToString<__int64>(number);
	}

	template <>
	inline tstring ToString<long>(long number)
	{
		return ToString<__int64>(number);
	}

	template <>
	inline tstring ToString<unsigned long>(unsigned long number)
	{
		return ToString<__int64>(number);
	}

	inline int CopyMemorySafely(void* target, int capacity, const void* source, int size) throw()
	{
		if (target == NULL || capacity <= 0 || source == NULL || size <= 0 )
		{
			return 0;
		}

		int nSize = capacity <= size ? capacity : size;
		memcpy(target, source, nSize);
		return nSize;
	}

	template <typename T>
	inline int CopyMemorySafely(T& t1, const T& t2) throw()
	{
		return CopyMemorySafely(&t1, sizeof(t1), &t2, sizeof(t2));
	}

	// 禁止如下用法，非常危险，所以放在这里不实现
	template <typename T1, typename T2>
	inline int CopyMemorySafely(T1& t1, const T2& t2) throw();

	template <typename T>
	inline int CopyMemorySafely(T& t, const void* source, int size) throw();

	inline int CopyMemorySafely(char* t, const void* source, int nSize) throw();

	inline int CopyMemorySafely(void* t, const void* source, int nSize) throw();

	/**
	 * 返回值为拷贝的字符数，不包括末尾的\0
	 */
	template <class T>
	inline int CopyStringSafely(T& target, const wchar_t* szSource) throw()
	{
		// Use cast to ensure that we only allow character arrays
		(static_cast<wchar_t[sizeof(target) / sizeof(wchar_t)]>(target));

		// Copy up to the size of the buffer
		return _CopyStringSafely(target, sizeof(target) / sizeof(wchar_t), szSource);
	}

	inline int _CopyStringSafely(wchar_t* szTarget, int nTargetSize, const wchar_t* szSource) throw()
	{
		if (szSource == 0)
		{
			return 0;
		}
		int nSourceSize = wcslen(szSource);
		assert(nTargetSize > nSourceSize);
		int nSize = nTargetSize <= nSourceSize ? nTargetSize - 1 : nSourceSize;
		memcpy(szTarget, szSource, nSize);
		szTarget[nSize] = '\0';
		return nSize;
	}

	/**
	 * 返回值为拷贝的字符数，不包括末尾的\0
	 */
	template <class T>
	inline int CopyStringSafely(T& target, const char* szSource) throw()
	{
		// Use cast to ensure that we only allow character arrays
		(static_cast<char[sizeof(target)]>(target));

		// Copy up to the size of the buffer
		return _CopyStringSafely(target, sizeof(target), szSource);
	}

	inline int _CopyStringSafely(char* szTarget, int nTargetSize, const char* szSource) throw()
	{
		if (szSource == 0)
		{
			return 0;
		}
		int nSourceSize = strlen(szSource);
		int nSize = nTargetSize <= nSourceSize ? nTargetSize - 1 : nSourceSize;
		memcpy(szTarget, szSource, nSize);
		szTarget[nSize] = '\0';
		return nSize;
	}

	size_t HexToBinrary(const char* hex, size_t length, char* binrary, size_t binrary_cap);
	size_t BinraryToHex(const char* binrary, size_t binrary_cap, char* hex, size_t length);

}

#endif
