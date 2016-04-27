#include "LuaHelper.h"
#include "Table.h"
#include "Utility.h"
#include "LuaFunctor.h"
#include "capi.h"
#include "Log.h"
#include "StringAlgorithm.h"
#include "Win32.h"
#include <Lua/lua.hpp>
#include "xxhash.h"
#include <boost/thread.hpp>
#pragma warning( push )
#pragma warning( disable : 4127)
#pragma warning( disable : 4996)
#include <boost/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/unordered/unordered_set.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/chrono.hpp>
#include <iostream>

namespace bc = boost::chrono;


scgl::CLuaHelper::LoadFuntion g_LuaLoader = NULL;

namespace scgl
{

	typedef struct LoadF
	{
		boost::filesystem::ifstream* stream;
		char buff[LUAL_BUFFERSIZE];
	} LoadF;

	char* load_lua_string(const char* strFullPath, int& nLen)
	{
		char* pBuf = NULL;
		if (g_LuaLoader)
		{
			std::string strBuffer = g_LuaLoader(strFullPath);
			if (strBuffer != "")
			{
				nLen = strBuffer.size();
				pBuf = new char[nLen + 1];
				memset(pBuf, 0, nLen + 1);
				memcpy(pBuf, strBuffer.c_str(), nLen);
			}
		}
		return pBuf;
	}

	struct stPacketLoader : public CLuaHelper::stSelfDefinedLoader
	{
	public:
		stPacketLoader() {}
		~stPacketLoader() {}

		bool load(const char* sFileName, const void*& pBuffer, int& nLen)
		{
			pBuffer = load_lua_string(sFileName, nLen);
			if (pBuffer)
			{
				return true;
			}
			return false;
		}
	} PacketExternalLoader;


	int load_lua_file(lua_State* L, const char* strFullPath, const char* name)
	{
		int nLen =  0;
		char* pBuf = load_lua_string(strFullPath, nLen);
		if (!pBuf)
		{
			return 0;
		}
		int nRet = 0;
		if (luaL_loadbuffer(L, (const char*)pBuf, nLen, name) == 0)
		{
			nRet = 1;
		}
		else
		{
			std::cerr << "load lua file failed:[" << strFullPath << "]" << luaL_checkstring(L, -1) << "\n";
		}
		delete[] pBuf;
		return nRet;
	}


	typedef std::vector<std::string> StringVector;
	int Require(lua_State* L)
	{
		static const char* strPath = "path";
		const char* name = luaL_checkstring(L, 1);

		name = luaL_gsub(L, name, ".", "/");

		lua_getglobal(L, LUA_LOADLIBNAME);
		if (lua_type(L, -1) != LUA_TTABLE)
		{
			return 1;
		}

		lua_pushstring(L, strPath);
		lua_gettable(L, -2);
		std::string scriptPath = luaL_checkstring(L, -1);

		StringVector searchPaths;
		boost::split(searchPaths, scriptPath, boost::is_any_of(";"));

		for (StringVector::const_iterator it = searchPaths.begin(); it != searchPaths.end(); ++it)
		{
			std::string strFullPath = *it;
			boost::replace_first(strFullPath, "?", name);
			if (strFullPath.empty())
			{
				std::cerr << "Search path error:[" << name << "]" << scriptPath << "\n";
				continue;
			}

			StringVector fullName;
			boost::split(fullName, strFullPath, boost::is_any_of("/"));
			if (fullName.empty())
			{
				std::cerr << "Get full file name error:[" << name << "]" << strFullPath << "\n";
				continue;
			}

			// 禁止加载外部文件
			if (load_lua_file(L, strFullPath.c_str(), fullName.rbegin()->c_str()))
			{
				return 1;
			}
		}

		std::cerr << "load lua file failed:[" << name << "]" << luaL_checkstring(L, -1) << "\n";

		return 0;
	}

	struct StringHash
	{
		unsigned int operator()(const std::string& value) const
		{
			return XXH32(value.c_str(), value.length(), 0);
		}
	};


	typedef boost::unordered_map<std::string, scgl::CLuaFunctor*, StringHash>	LuaFunctorMap;

	struct SCallCountInfo
	{
		unsigned long long	amount;
		bc::nanoseconds		time;
		bc::nanoseconds		maxTime;
		SCallCountInfo(const bc::nanoseconds _time)
			: amount(1)
			, time(_time)
			, maxTime(_time)
		{

		}
	};

	struct VariantHash
	{
		unsigned int operator()(const scgl::CVariant& value) const
		{
			if (value.GetMetaType() == CVariant::TYPE_INTEGER)
			{
				long long v = value.GetNumber<long long>();
				return XXH32(&v, sizeof(v), 0);
			}
			else if (value.GetMetaType() == CVariant::TYPE_STRING)
			{
				return XXH32(value.GetString(), value.GetLength(), 0);
			}

			return 0;
		}


	};

	typedef boost::unordered_map<scgl::CVariant, SCallCountInfo, VariantHash>	CallCountInfoMap;

	struct CLuaHelper::Impl
	{
		LuaFunctorMap		luaFunctors;
		scgl::CLuaFunctor*	Process;
		bool				mainLoaded;
		bool				countProcess;
		CallCountInfoMap	processCountInfoList;		// process调用函数性能统计
		GetStrByIntF		GetProcessIDName;
		Impl()
			: Process(NULL)
			, mainLoaded(false)
#if defined(_DEBUG) || defined(DEBUG)
			, countProcess(true)
#else
			, countProcess(false)
#endif
			, GetProcessIDName(NULL)
		{

		}
	};

	CLuaHelper::CLuaHelper()
		: m_impl(new Impl)
		, m_pMainThread(NULL)
		, m_bChunkLoaded(false)
		, m_pExternalLuaLoader(NULL)
	{
		CreateMainState();
	}

	CLuaHelper::~CLuaHelper(void)
	{
		for (LuaFunctorMap::iterator it = m_impl->luaFunctors.begin(); it !=  m_impl->luaFunctors.end(); ++it)
		{
			delete it->second;
		}
		m_impl->luaFunctors.clear();
		ReleaseMainState();

		delete m_impl;
	}

	void CLuaHelper::Clear()
	{
		ReleaseMainState();
		m_bChunkLoaded = false;
		CreateMainState();
	}

	bool CLuaHelper::LoadFile(const char* file)
	{
		m_bChunkLoaded = false;

		int n = lua_gettop(m_pMainThread);

		if (!file)
		{
			return m_bChunkLoaded;
		}

		if (IsStackFull())
		{
			return false;
		}

		if (!m_bChunkLoaded && m_pExternalLuaLoader != NULL)
		{
			const void* pBuf = NULL;
			int nSize = 0;

			if (m_pExternalLuaLoader->load(file, pBuf, nSize))
			{
				if (luaL_loadbuffer(m_pMainThread, (const char*)pBuf, nSize, file) == 0)
				{
					m_bChunkLoaded = true;
				}
			}
			if (pBuf)
			{
				delete[] pBuf;
			}
		}
		else if (luaL_loadfile(m_pMainThread, file) == 0)
		{
			m_bChunkLoaded = true;
		}


		if (!m_bChunkLoaded)
		{
			ClearStackTempData(n);
		}

		return m_bChunkLoaded;
	}

	bool CLuaHelper::Run()
	{
		if (!m_bChunkLoaded)
		{
			return false;
		}

		int n = lua_gettop(m_pMainThread);
		LOG_TRACE("Lua", __FUNCTION__ << boost::this_thread::get_id() << ",this:" << this << ",L:" << m_pMainThread);
		if (lua_pcall(m_pMainThread, 0, LUA_MULTRET, 0))
		{
			std::cerr << luaL_checkstring(m_pMainThread, -1) <<  "\n";
			ClearStackTempData(n);
			return false;
		}
		ClearStackTempData(n);
		return true;
	}

	bool CLuaHelper::RunFile(const char* file)
	{
		VariantVector parameters;
		parameters.push_back(file);
		if (!m_impl->mainLoaded)
		{
			if ((LoadFile("../script/main.lua") || LoadFile("./script/main.lua")) && Run())
			{
				m_impl->mainLoaded = true;
			}
			else
			{
				return false;
			}
		}
		return CallFunction("main", &parameters);
	}

	bool CLuaHelper::RunFileOrigin(const char* file)
	{
		return LoadFile(file) && Run();
	}

	bool CLuaHelper::RunString( const char* string )
	{
		if (luaL_dostring(m_pMainThread, string))
		{
			std::cerr << luaL_checkstring(m_pMainThread, -1) <<  "\n";
			return false;
		}
		return true;
	}

	bool CLuaHelper::PushTable(CTable& table)
	{
		if (IsStackFull())
		{
			return false;
		}

		table.PushIntoLua(m_pMainThread);
		return true;
	}

	bool CLuaHelper::PushNumber(const char* globalName, double number)
	{
		if (IsStackFull())
		{
			return false;
		}

		int n = lua_gettop(m_pMainThread);
		lua_pushnumber(m_pMainThread, number);
		lua_setglobal(m_pMainThread, globalName);
		ClearStackTempData(n);
		return true;
	}

	bool CLuaHelper::PushString(const char* globalName, const char* value)
	{
		if (IsStackFull())
		{
			return false;
		}

		int n = lua_gettop(m_pMainThread);
		lua_pushstring(m_pMainThread, value);
		lua_setglobal(m_pMainThread, globalName);
		ClearStackTempData(n);
		return true;
	}

	bool CLuaHelper::PushInteger(const char* globalName, long long number)
	{
		if (IsStackFull())
		{
			return false;
		}

		int n = lua_gettop(m_pMainThread);
		lua_pushinteger(m_pMainThread, LUA_INTEGER(number));
		lua_setglobal(m_pMainThread, globalName);
		ClearStackTempData(n);
		return true;
	}

	bool CLuaHelper::GetNumber(const char* globalName, double& number)
	{
		if (IsStackFull())
		{
			return false;
		}

		int n = lua_gettop(m_pMainThread);
		lua_getglobal(m_pMainThread, globalName);
		if (lua_type(m_pMainThread, -1) == LUA_TNUMBER)
		{
			number = luaL_checknumber(m_pMainThread, -1);
			ClearStackTempData(n);
			return true;
		}
		ClearStackTempData(n);
		return false;
	}

	bool CLuaHelper::CallFunction(const char* functionName, const VariantVector* params, VariantVector* results)
	{
		if (!functionName)
		{
			return false;
		}

		LuaFunctorMap::iterator function = m_impl->luaFunctors.find(functionName);
		if (function == m_impl->luaFunctors.end())
		{
			int n = lua_gettop(m_pMainThread);
			lua_getglobal(m_pMainThread, functionName);
			if (!lua_isfunction(m_pMainThread, -1))
			{
				ClearStackTempData(n);
				return false;
			}
			CLuaFunctor* functor = new CLuaFunctor(m_pMainThread, -1);
			function = m_impl->luaFunctors.insert(std::make_pair(std::string(functionName), functor)).first;
			ClearStackTempData(n);
		}

		return (*function->second)(params, results);
	}

	bool CLuaHelper::Process(const VariantVector* params, VariantVector* results)
	{
		if (m_impl->Process == NULL)
		{
			LuaFunctorMap::iterator function = m_impl->luaFunctors.find("Process");
			if (function == m_impl->luaFunctors.end())
			{
				return CallFunction("Process", params, results);
			}
			else
			{
				m_impl->Process = function->second;
			}
		}

		bc::high_resolution_clock::time_point start;
		if (m_impl->countProcess && !params->empty())
		{
			start = bc::high_resolution_clock::now();
		}
		bool result = (*m_impl->Process)(params, results);
		if (m_impl->countProcess && !params->empty())
		{
			const CVariant& key = params->at(0);
			CallCountInfoMap::iterator it = m_impl->processCountInfoList.find(key);
			if (it != m_impl->processCountInfoList.end())
			{
				SCallCountInfo& callCountInfo = it->second;
				bc::nanoseconds duration = bc::high_resolution_clock::now() - start;
				callCountInfo.time += duration;
				if (duration > callCountInfo.maxTime)
				{
					callCountInfo.maxTime = duration;
				}
				++(callCountInfo.amount);
			}
			else
			{
				m_impl->processCountInfoList.insert(std::make_pair(key, SCallCountInfo(bc::high_resolution_clock::now() - start)));
			}
		}

		return result;

	}

	void CLuaHelper::SetLoadFunction(LoadFuntion loader)
	{
		m_pExternalLuaLoader = &PacketExternalLoader;
		g_LuaLoader = loader;

		if (Require != NULL)
		{
			lua_getglobal(m_pMainThread, LUA_LOADLIBNAME);
			if (!lua_istable(m_pMainThread, -1))
			{
				return;
			}

			lua_pushstring(m_pMainThread, "loaders");
			lua_gettable(m_pMainThread, -2);

			if (lua_istable(m_pMainThread, -1))
			{
				lua_pushcfunction(m_pMainThread, Require);
				lua_rawseti(m_pMainThread, -2, 2);
			}
		}

	}

	void CLuaHelper::CreateMainState()
	{
		m_pMainThread = luaL_newstate();
		luaL_openlibs(m_pMainThread);
		lua_pushthread(m_pMainThread);
		lua_setglobal(m_pMainThread, "g_mainState");
#ifdef _DEBUG
		PushBoolean("DEBUG", true);
#endif
	}

	void CLuaHelper::ReleaseMainState()
	{
		if (m_pMainThread != NULL)
		{
			lua_close(m_pMainThread);
			m_pMainThread = NULL;
		}
	}

	void CLuaHelper::ClearStackTempData(int nTempDataAmount)
	{
		lua_pop(m_pMainThread, lua_gettop(m_pMainThread) - nTempDataAmount);
	}

	bool CLuaHelper::IsStackFull()
	{
		return !HaveFreeStackSlot(1);
	}

	bool CLuaHelper::HaveFreeStackSlot(int nFreeSlot)
	{
		if (lua_checkstack(m_pMainThread, nFreeSlot))
		{
			return true;
		}
		return false;
	}

	bool CLuaHelper::IsChunkLoaded() const
	{
		return m_bChunkLoaded;
	}

	bool CLuaHelper::PushFunction(const char* globalName, lua_CFunction function)
	{
		if (IsStackFull())
		{
			return false;
		}

		int n = lua_gettop(m_pMainThread);
		lua_pushcfunction(m_pMainThread, function);
		lua_setglobal(m_pMainThread, globalName);
		ClearStackTempData(n);
		return true;
	}

	bool CLuaHelper::PushBoolean(const char* globalName, bool boolean)
	{
		if (IsStackFull())
		{
			return false;
		}

		int n = lua_gettop(m_pMainThread);
		lua_pushboolean(m_pMainThread, boolean);
		lua_setglobal(m_pMainThread, globalName);
		ClearStackTempData(n);
		return true;
	}

	void CLuaHelper::SetNameFunction(GetStrByIntF f)
	{
		m_impl->GetProcessIDName = f;
	}

	bool CLuaHelper::SetProcessCount( bool countProcess )
	{
		m_impl->countProcess = countProcess;
		return true;
	}

	bool CLuaHelper::ClearProcessCountInfo()
	{
		m_impl->processCountInfoList.clear();
		return true;
	}

	bool CLuaHelper::LogProcessCountInfo() const
	{
		if (m_impl->GetProcessIDName == NULL)
		{
			return false;
		}

		for (CallCountInfoMap::const_iterator i = m_impl->processCountInfoList.begin(); i != m_impl->processCountInfoList.end(); ++i)
		{
			const CVariant& key = i->first;
			const SCallCountInfo& callCountInfo = i->second;
			LOG_DEBUG("Profmon", "PROCESS_COUNT_INFO\t" << (key.GetMetaType() == CVariant::TYPE_INTEGER ? m_impl->GetProcessIDName(key.GetNumber<int>()) : key.GetString())
				<< "\tCALL_AMOUNT\t" << callCountInfo.amount
				<< "\tTOTAL\t" << callCountInfo.time.count()
				<< "\tMAX\t" << callCountInfo.maxTime.count()
				<< "\tAVERAGE\t" << (callCountInfo.amount > 0 ? callCountInfo.time.count() / callCountInfo.amount : -1));
		}

		return true;
	}


#pragma warning( pop )

}
