#ifndef __CRASH_REPORTER_H
#define __CRASH_REPORTER_H

#include "Win32.h"
#include <excpt.h>

namespace scgl
{

	// Possible actions to take on a crash.  If you want to restart the app as well, see the CrashRelauncher sample.
	enum CrashReportAction
	{
		// Write the minidump to disk in a specified directory
		AOC_WRITE_BIG_DUMP = 1,
		AOC_SILENT_MODE = 2,
	};

	/// Holds all the parameters to CrashReporter::Start
	struct CrashReportControls
	{
		int actionToTake;
		CrashReportControls();
	};

	class CrashReporter
	{
	public:
		static void Start(CrashReportControls* input);
		static CrashReportControls controls;
	};


	LONG DumpException(struct _EXCEPTION_POINTERS* exceptionInfo, const char* dumpName);
	LONG WINAPI HandleException(struct _EXCEPTION_POINTERS* exceptionInfo);

	typedef void (*ProcessDumpRoutine)(const char* dumpName, HANDLE fileHandle);
	void SetProcessDumpRoutine(ProcessDumpRoutine f);
	typedef void (*SendClientErrorLogRoutine)(const char* strError);
	void SetSendClientErrorLogRoutine(SendClientErrorLogRoutine f);
	void SendClientErrorLog(const char* strError);
}

#endif
