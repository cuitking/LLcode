#ifndef SCGL_INIFILE_H
#define SCGL_INIFILE_H

#include "Constant.h"
#include "tiostream"
#include <tchar.h>
#include <string>
#include <vector>
#include <map>

namespace scgl
{

	class CIniFile
	{
	public:
		typedef std::vector<std::pair<std::string, std::string> >  StringPairVector;

	public:
		CIniFile(const TCHAR* file);
		~CIniFile();
	public:
		tstring Read(const TCHAR* section, const TCHAR* key, const TCHAR* defaultValue = NULL_STRING) const;
		int ReadInteger(const TCHAR* section, const TCHAR* key, int defaultValue = 0) const;
		bool IsTrue(const TCHAR* section, const TCHAR* key) const;
		bool Write(const TCHAR* section, const TCHAR* key, const TCHAR* value) const;
		StringPairVector ReadSection(const TCHAR* section) const;

	private:
		void ClearBuffer() const;
	private:
		static const int    MAX_STRING_LENGTH = 4096;
		static const int    MAX_PATH_LENGTH = 260;
		tstring				m_file;
		mutable TCHAR       m_szBuffer[MAX_STRING_LENGTH];
		static const int    BUFFER_SIZE = sizeof(TCHAR)* MAX_STRING_LENGTH;
	};


}
#endif
