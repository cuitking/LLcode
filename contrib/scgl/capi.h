#ifndef CAPI_H
#define CAPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <wchar.h>

	/**
	 * 显示系统错误消息框（WIN32）
	 */
	void ShowSystemErrorMessage();

	const char* GetCurrentProcessName();

	void SetConsoleCodePageToUtf8();

	const char* UnicodeToUTF8(const wchar_t* strUnicode);
	const wchar_t* UTF8ToUnicode(const char* strUTF8);

	const char* NativeToUTF8(const char* strUnicode);
	const char* UTF8ToNative(const char* strUTF8);

	void DisableStickyKeysConfirm(); 

#ifdef __cplusplus
}
#endif

#endif