#include "capi.h"
#include "Win32.h"

void ShowSystemErrorMessage()
{
	TCHAR szMessageBuffer[4096];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
				  NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				  szMessageBuffer, sizeof(szMessageBuffer), NULL);
	MessageBox(GetConsoleWindow(), szMessageBuffer, NULL, MB_OK);
}

const char* GetCurrentProcessName()
{
	static char processName[MAX_PATH] = { 0 };
	char szPath[MAX_PATH];

	if (processName[0] != 0)
	{
		return processName;
	}

	if (!GetModuleFileName(NULL, szPath, sizeof(szPath)))
	{
		return NULL;
	}

	strcpy(processName, strrchr(szPath, '\\') + 1);
	*(strrchr(processName, '.')) = 0;
	return processName;
}

void SetConsoleCodePageToUtf8()
{
	if (!SetConsoleCP(CP_UTF8))
	{
		ShowSystemErrorMessage();
	}
	if (!SetConsoleOutputCP(CP_UTF8))
	{
		ShowSystemErrorMessage();
	}
}

#define MAX_UTF8_LENGTH 8192

const char* UnicodeToUTF8(const wchar_t* unicode)
{
	static char utf8[MAX_UTF8_LENGTH];
	memset(utf8, 0, sizeof(utf8));
	WideCharToMultiByte(CP_UTF8, 0, unicode, -1, utf8, sizeof(utf8), NULL, NULL);
	return utf8;
}

const wchar_t* UTF8ToUnicode(const char* utf8)
{
	static wchar_t unicode[MAX_UTF8_LENGTH];
	memset(unicode, 0, sizeof(unicode));
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, unicode, sizeof(unicode));
	return unicode;
}

const char* NativeToUTF8(const char* native)
{
	static wchar_t unicode[MAX_UTF8_LENGTH];
	memset(unicode, 0, sizeof(unicode));

	MultiByteToWideChar(CP_OEMCP, 0, native, -1, unicode, sizeof(unicode));
	return UnicodeToUTF8(unicode);
}

const char* UTF8ToNative(const char* utf8)
{
	static char native[MAX_UTF8_LENGTH];
	memset(native, 0, sizeof(native));
	WideCharToMultiByte(CP_OEMCP, 0, UTF8ToUnicode(utf8), -1, native, sizeof(native), NULL, NULL);
	return native;
}

void DisableStickyKeysConfirm()
{
	STICKYKEYS stickyKeys = {sizeof(STICKYKEYS), 0};
	SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), &stickyKeys, 0);

	// Disable the hotkey and the confirmation
	stickyKeys.dwFlags &= ~SKF_HOTKEYACTIVE;
	stickyKeys.dwFlags &= ~SKF_CONFIRMHOTKEY;

	SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &stickyKeys, 0);
}
