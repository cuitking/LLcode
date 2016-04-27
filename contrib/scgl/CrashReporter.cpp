#include "CrashReporter.h"
#include "StringAlgorithm.h"
#include "Utility.h"
#include <stdio.h>
#include <boost/thread/mutex.hpp>
#include <stdlib.h>
#include <DbgHelp.h>
#include <Psapi.h>

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "Psapi.lib")

namespace scgl
{
	ProcessDumpRoutine ProcessDump = NULL;
	SendClientErrorLogRoutine SendClientErrorLogFun = NULL;

	scgl::CrashReportControls scgl::CrashReporter::controls;

	void CrashReporter::Start(CrashReportControls* input)
	{
		memcpy(&controls, input, sizeof(CrashReportControls));
		SetUnhandledExceptionFilter(HandleException);
	}

	CrashReportControls::CrashReportControls()
	{
		actionToTake = 0;
	}

	// 由于创建目录需要错误判断，增大复杂度，因此不再创建目录，直接生成到当前目录，也利于玩家找到dmp

	// 可能被任何线程调用，内部使用static变量，减少内存分配引起错误的可能性
	LONG DumpException( struct _EXCEPTION_POINTERS* exceptionInfo, const char* dumpName )
	{
		static boost::mutex m;
		boost::mutex::scoped_lock l(m);

		static HANDLE tempFileHandle;
		tempFileHandle = INVALID_HANDLE_VALUE;

		static bool success;
		success = false;

		static const int MAX_TRY_DUMP = 8;		// 创建失败重试的次数
		static int i;
		for (i = 0; i < MAX_TRY_DUMP; ++i)
		{
			::Sleep(1000);

			// 首先生成文件
			if (tempFileHandle == INVALID_HANDLE_VALUE)
			{
				static SYSTEMTIME st;
				GetLocalTime(&st);

				static char fileName[MAX_PATH];
				sprintf_s(fileName, sizeof(fileName), "%s_%04d%02d%02d_%02d%02d%02d.dmp", dumpName, st.wYear, st.wMonth, st.wDay,
					st.wHour, st.wMinute, st.wSecond);

				tempFileHandle = CreateFile(fileName
					, GENERIC_READ | GENERIC_WRITE
					, 0
					, NULL
					, CREATE_ALWAYS
					, 0
					, NULL);
				if (tempFileHandle == INVALID_HANDLE_VALUE)
				{
					continue;
				}
			}

			static MINIDUMP_EXCEPTION_INFORMATION minidumpExceptionInfo;
			memset(&minidumpExceptionInfo, 0, sizeof(minidumpExceptionInfo));
			minidumpExceptionInfo.ThreadId = GetCurrentThreadId();
			minidumpExceptionInfo.ExceptionPointers = exceptionInfo;
			minidumpExceptionInfo.ClientPointers = FALSE;

			if (MiniDumpWriteDump(
				GetCurrentProcess(),
				GetCurrentProcessId(),
				tempFileHandle,
				MiniDumpNormal,
				&minidumpExceptionInfo,
				NULL,
				NULL))
			{
				success = true;
				break;
			}
		}

		if (success)
		{
			if (ProcessDump != NULL)
			{
				ProcessDump(dumpName, tempFileHandle);
			}

			CloseHandle(tempFileHandle);
			return EXCEPTION_EXECUTE_HANDLER;
		}

		CloseHandle(tempFileHandle);
		return EXCEPTION_CONTINUE_SEARCH;
	}

	void SetProcessDumpRoutine( ProcessDumpRoutine f )
	{
		ProcessDump = f;
	}

	void SetSendClientErrorLogRoutine(SendClientErrorLogRoutine f)
	{
		SendClientErrorLogFun = f;
	}

	LONG WINAPI DumpFull(struct _EXCEPTION_POINTERS* exceptionInfo);

	LONG WINAPI HandleException(struct _EXCEPTION_POINTERS* exceptionInfo)
	{
		LONG retVal = EXCEPTION_CONTINUE_SEARCH;
		if (exceptionInfo == NULL)
		{
			__try
			{
				RaiseException(EXCEPTION_BREAKPOINT, 0, 0, NULL);
			}
			__except (HandleException(GetExceptionInformation()))
			{
			}
		}
		else
		{
			retVal = DumpFull(exceptionInfo);

		}
		return retVal;
	}

	// 可能被任何线程调用，内部使用static变量，减少内存分配引起错误的可能性
	LONG WINAPI DumpFull(struct _EXCEPTION_POINTERS* exceptionInfo)
	{
		static boost::mutex m;
		boost::mutex::scoped_lock l(m);

		static char moduleName[MAX_PATH];
		static DWORD length;
		length = GetModuleBaseName(GetCurrentProcess(), NULL, moduleName, sizeof(moduleName));
		if (length <= 0)
		{
			scgl::CopyStringSafely(moduleName, "UNKNOWN");
		}

		DumpException(exceptionInfo, moduleName);

		// 设置BIG_DUMP或者弹出询问时生成BIG_DUMP
		if (scgl::CrashReporter::controls.actionToTake & scgl::AOC_WRITE_BIG_DUMP)
		{
			if (scgl::CrashReporter::controls.actionToTake & AOC_SILENT_MODE)
			{
			}
			else if (::MessageBox(GetActiveWindow(), "请不要关闭此对话框，尽快与程序联系！\n\n是否生成完整内存dump？", "Crash Reporter", MB_YESNO | MB_DEFBUTTON2) == IDNO)
			{
				return TerminateProcess(GetCurrentProcess(), exceptionInfo->ExceptionRecord->ExceptionCode);
			}

			static SYSTEMTIME st;
			GetLocalTime(&st);

			static char fileName[MAX_PATH];
			sprintf(fileName, TEXT("%s_%04d%02d%02d_%02d%02d%02d_big.dmp"), moduleName, st.wYear, st.wMonth, st.wDay,
				st.wHour, st.wMinute, st.wSecond);

			static HANDLE dumpFile;
			dumpFile = CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (dumpFile == INVALID_HANDLE_VALUE)
			{
				return EXCEPTION_CONTINUE_SEARCH;
			}

			static MINIDUMP_EXCEPTION_INFORMATION minidumpExceptionInfo;
			memset(&minidumpExceptionInfo, 0, sizeof(minidumpExceptionInfo));
			minidumpExceptionInfo.ThreadId = GetCurrentThreadId();
			minidumpExceptionInfo.ExceptionPointers = exceptionInfo;
			minidumpExceptionInfo.ClientPointers = FALSE;

			if (!MiniDumpWriteDump(
				GetCurrentProcess(),
				GetCurrentProcessId(),
				dumpFile,
				MiniDumpWithFullMemory,
				exceptionInfo ? &minidumpExceptionInfo : NULL,
				NULL,
				NULL))
			{
				CloseHandle(dumpFile);
				return EXCEPTION_CONTINUE_SEARCH;
			}

			CloseHandle(dumpFile);
		}

		return TerminateProcess(GetCurrentProcess(), exceptionInfo->ExceptionRecord->ExceptionCode);
	}

	void SendClientErrorLog(const char* strError)
	{
		if(SendClientErrorLogFun)
		{
			SendClientErrorLogFun(strError);
		}
	}
}

