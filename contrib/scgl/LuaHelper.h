#ifndef SCGL_LUA_HELPER_H
#define SCGL_LUA_HELPER_H

#include <vector>
#include <scgl/SingletonHolder.h>

typedef struct lua_State lua_State;
typedef int (*lua_CFunction)(lua_State* L);
typedef const char* (*GetStrByIntF)(int value);

namespace scgl
{

	class CVariant;
	typedef std::vector<CVariant> VariantVector;
	class CTable;


	class CLuaHelper
	{
	public:
		CLuaHelper();
		~CLuaHelper(void);
	public:
		void Clear();
		bool IsChunkLoaded() const;
		bool LoadFile(const char* file);
		bool Run();
		bool RunFile(const char* file);
		bool RunString(const char* string);
		bool RunFileOrigin(const char* file);
		bool PushFunction(const char* globalName, lua_CFunction function);
		bool PushTable(CTable& pTable);
		bool PushNumber(const char* globalName, double number);
		bool PushInteger(const char* globalName, long long number);
		bool PushBoolean(const char* globalName, bool boolean);
		bool PushString(const char* globalName, const char* value);
		bool GetNumber(const char* globalName, double& number);
		bool Process(const VariantVector* params = NULL, VariantVector* results = NULL);
		bool CallFunction(const char* functionName, const VariantVector* params = NULL, VariantVector* results = NULL);
		lua_State* GetLuaState()
		{
			return m_pMainThread;
		}
		typedef std::string(*LoadFuntion)(const char* sFileName);
		void SetLoadFunction(LoadFuntion loader);

		void ReleaseMainState();
		void SetNameFunction(GetStrByIntF f);
		bool SetProcessCount(bool countProcess);
		bool ClearProcessCountInfo();
		bool LogProcessCountInfo() const;


	public:

		//自定义loader
		struct stSelfDefinedLoader
		{
		public:
			stSelfDefinedLoader()
			{
				m_pBuf = NULL;
			}
			~stSelfDefinedLoader()
			{
				delete []m_pBuf;
				m_pBuf = NULL;
			}

			virtual bool load(const char* sFileName, const void*& pBuffer, int& nLen) = 0;

		protected:
			void* m_pBuf;
		};

		void SetLoader(stSelfDefinedLoader* pLoader)
		{
			m_pExternalLuaLoader = pLoader;
		}


	private:
		template <typename T> friend class CLuaBinder;

	private:
		CLuaHelper(const CLuaHelper&);
		const CLuaHelper& operator=(const CLuaHelper&);

	private:
		void CreateMainState();
		void ClearStackTempData(int nTempDataAmount);
		bool IsStackFull();
		bool HaveFreeStackSlot(int nFreeSlot);
	private:

		lua_State*  m_pMainThread;
		bool        m_bChunkLoaded;
		stSelfDefinedLoader* m_pExternalLuaLoader;//C_API的自定义载入模块

		struct	Impl;
		Impl*	m_impl;

	};

	typedef scgl::SingletonHolder< CLuaHelper > SLuaHelper;
}

#endif
