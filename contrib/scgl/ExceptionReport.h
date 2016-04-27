#pragma once
#include <Windows.h>
#include <tchar.h>
#include <algorithm>
#include <DbgHelp.h>

/*
崩溃处理类. 如果有崩溃发生,该类会自动接管处理流程

用法: 定义一个全局的类对象,例如: ingest::ExceptionReport g_exceptionReport;

默认情况下,崩溃发生后会退出,没有任何提示,应用程序可以在崩溃发生后得到通知,方法是:

1.定义一个全局或静态函数: void CALLBACK FUNCProcessCrash();
2.调用方法ingest::ExceptionReport::SetProcessCrashFunc()设置这个函数,崩溃发生后该函数会被调用
3.在FUNCProcessCrash()中编写自己的处理流程, 例如可以弹出一个提示对话框(客户端使用),
  或者自动重新启动程序(服务端使用),或者弹出一个调试对话框等
4.崩溃发生后,该类会自动记录崩溃发生的场景,包括错误地址,发生的模块,CPU状态和调用堆栈等,
  这些信息记录在应用程序目录下的crashdump目录下面
*/

namespace scgl
{

	enum BasicType  // Stolen from CVCONST.H in the DIA 2.0 SDK
	{
		btNoType = 0,
		btVoid = 1,
		btChar = 2,
		btWChar = 3,
		btInt = 6,
		btUInt = 7,
		btFloat = 8,
		btBCD = 9,
		btBool = 10,
		btLong = 13,
		btULong = 14,
		btCurrency = 25,
		btDate = 26,
		btVariant = 27,
		btComplex = 28,
		btBit = 29,
		btBSTR = 30,
		btHresult = 31
	};

	typedef BOOL (_stdcall* FUNCSymFromAddr)(
		IN HANDLE hProcess,
		IN DWORD64 Address,
		OUT PDWORD64 Displacement,
		IN OUT PSYMBOL_INFO Symbol
	);
	typedef DWORD (_stdcall* FUNCSymGetOptions)();
	typedef DWORD (_stdcall* FUNCSymSetOptions)(
		IN DWORD SymOptions
	);
	typedef BOOL (_stdcall* FUNCSymCleanup)(
		IN HANDLE hProcess
	);
	typedef BOOL (_stdcall* FUNCSymInitialize)(
		IN HANDLE hProcess,
		IN PSTR UserSearchPath,
		IN BOOL fInvadeProcess
	);
	typedef BOOL (_stdcall* FUNCSymEnumSymbols)(
		IN HANDLE hProcess,
		IN ULONG64 BaseOfDll,
		IN PCSTR Mask,
		IN PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
		IN PVOID UserContext
	);
	typedef ULONG(_stdcall* FUNCSymSetContext)(
		HANDLE hProcess,
		PIMAGEHLP_STACK_FRAME StackFrame,
		PIMAGEHLP_CONTEXT Context
	);
	typedef BOOL (_stdcall* FUNCSymGetLineFromAddr)(
		IN HANDLE hProcess,
		IN DWORD dwAddr,
		OUT PDWORD pdwDisplacement,
		OUT PIMAGEHLP_LINE Line
	);
	typedef BOOL (_stdcall* FUNCStackWalk)(
		DWORD							  MachineType,
		HANDLE                            hProcess,
		HANDLE                            hThread,
		LPSTACKFRAME                      StackFrame,
		PVOID                             ContextRecord,
		PREAD_PROCESS_MEMORY_ROUTINE      ReadMemoryRoutine,
		PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccessRoutine,
		PGET_MODULE_BASE_ROUTINE          GetModuleBaseRoutine,
		PTRANSLATE_ADDRESS_ROUTINE        TranslateAddress
	);
	typedef PVOID(_stdcall* FUNCSymFunctionTableAccess)(
		HANDLE  hProcess,
		DWORD   AddrBase
	);
	typedef DWORD (_stdcall* FUNCSymGetModuleBase)(
		IN HANDLE     hProcess,
		IN DWORD      dwAddr
	);
	typedef BOOL (_stdcall* FUNCSymGetTypeInfo)(
		IN HANDLE          hProcess,
		IN DWORD64         ModBase,
		IN ULONG           TypeId,
		IN IMAGEHLP_SYMBOL_TYPE_INFO GetType,
		OUT PVOID           pInfo
	);
	typedef BOOL (_stdcall* FUNCMiniDumpWriteDump)(
		HANDLE hProcess,
		DWORD ProcessId,
		HANDLE hFile,
		MINIDUMP_TYPE DumpType,
		PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
		PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
		PMINIDUMP_CALLBACK_INFORMATION CallbackParam
	);

	typedef void (CALLBACK* FUNCProcessCrash)();

	class ExceptionReport
	{
	public:
		ExceptionReport(void);
		~ExceptionReport(void);

		static void SetProcessCrashFunc(FUNCProcessCrash func)
		{
			m_funcProcessCrash = func;
		}

	protected:
		static LONG WINAPI UnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo);

		// where report info is extracted and generated
		static void GenerateExceptionReport(PEXCEPTION_POINTERS pExceptionInfo);

		static LPTSTR GetExceptionString(DWORD dwCode);

		static BOOL GetLogicalAddress(
			PVOID addr,
			PTSTR szModule,
			DWORD len,
			DWORD& section,
			DWORD& offset
		);

		static void WriteStackDetails(PCONTEXT pContext, bool bWriteVariables);

		static BOOL CALLBACK EnumerateSymbolsCallback(PSYMBOL_INFO, ULONG, PVOID);

		static bool FormatSymbolValue(PSYMBOL_INFO, STACKFRAME*, TCHAR* pszBuffer, unsigned cbBuffer);

		static TCHAR* DumpTypeIndex(TCHAR*, DWORD64, DWORD, unsigned, DWORD_PTR, bool&);

		static BasicType GetBasicType(DWORD typeIndex, DWORD64 modBase);

		static TCHAR* FormatOutputValue(TCHAR* pszCurrBuffer, BasicType basicType, DWORD64 length, PVOID pAddress);

		static bool WriteMiniDump(PEXCEPTION_POINTERS pExceptionInfo);

		static int __cdecl _tprintf(const TCHAR* format, ...);

	protected:
		static TCHAR						m_szLogFileName[MAX_PATH];
		static TCHAR						m_szDmpFileName[MAX_PATH];
		static LPTOP_LEVEL_EXCEPTION_FILTER m_previousFilter;
		static HANDLE						m_hProcess;
		static HANDLE						m_hReportFile;
		static HANDLE						m_hDumpFile;
		static FUNCProcessCrash				m_funcProcessCrash;
	};

}