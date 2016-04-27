#ifndef TRANSFORM
#define TRANSFORM

#include "Win32.h"
#include <map>
using namespace std;

class CUtf8String
{
public:
	inline CUtf8String(const char* gb2312)
	{
		int buffLen = 0;
		WCHAR wbuff[5120];
		MultiByteToWideChar(CP_ACP, 0, gb2312, -1, wbuff, 5120);
		buffLen = WideCharToMultiByte(CP_UTF8, 0, wbuff, -1, NULL, 0, 0, 0);
		m_utf8 = new char[buffLen + 1];
		WideCharToMultiByte(CP_UTF8, 0, wbuff, -1, (LPSTR)m_utf8, buffLen, 0, 0);
	}

	inline ~CUtf8String()
	{
		if (m_utf8)
		{
			delete[] m_utf8;
			m_utf8 = 0;
		}
	}

	inline operator char* ()
	{
		return (char*)m_utf8;
	}
private:
	const char* m_utf8;
};

class CGb2312String
{
public:
	inline CGb2312String(const char* utf8)
	{
		int buffLen = 0;
		WCHAR wbuff[5120];
		MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wbuff, 5120);
		buffLen = WideCharToMultiByte(CP_ACP, 0, wbuff, -1, NULL, 0, 0, 0);
		m_gb2312 = new char[buffLen + 1];
		WideCharToMultiByte(CP_ACP, 0, wbuff, -1, (LPSTR)m_gb2312, buffLen, 0, 0);
	}

	inline ~CGb2312String()
	{
		if (m_gb2312)
		{
			delete[] m_gb2312;
			m_gb2312 = 0;
		}
	}

	inline operator char* ()
	{
		return (char*)m_gb2312;
	}
private:
	const char* m_gb2312;
};

#endif
