/*
** LuaProfiler
*/

/*****************************************************************************
lua50_profiler.c:
Lua version dependent profiler interface
*****************************************************************************/


#include "dict.h"
#include "performance_counter.h"
#include "checker.h"

#include "lua.h"
#include "lauxlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct _profiler_info
{
	long long	last_call;
	long long	total_with_children;
	long long	total;
	long long	max;
	size_t		count;
	int			linedefined;
	const char*	source;
	char		name[32];
} profiler_info;
const size_t PROFILER_INFO_SIZE = sizeof(profiler_info);
#define PROFILER_INFO_ARRAY_SIZE (2048)
profiler_info profiler_info_array[PROFILER_INFO_ARRAY_SIZE];
size_t current_profiler_position = 0;
dict* profiler_dictionary = NULL;
int backtracking = 0;
FILE* profiler_log_file = NULL;
char tab[128];
size_t tab_amount = 0;
size_t write_line_amount = 0;

typedef enum breakpoint_type
{
	breakpoint_type_normal = 0,
	breakpoint_type_condition = 1,
	breakpoint_type_temp = 2,
} breakpoint_type;
typedef struct breakpoint_info
{
	int				type;					// 断点类型
	int				line;					// 断点行号
	char			file_name[192];			// 断点文件
	int				conditionFunction;		// 条件断点表达式函数
} breakpoint_info;
size_t breakpoint_info_size = sizeof(breakpoint_info);

int break_callback = 0;
breakpoint_type bp_type = breakpoint_type_normal;
dict* breakpoint_dictionary = NULL;


__forceinline const char* trim_path(const char* full_path)
{
	const char* left_file_name = NULL;
	const char* right_file_name = NULL;
	if (full_path == NULL)
	{
		return NULL;
	}

	// 需要处理路径中既有/又有\的情况。所以取最短值
	left_file_name =  strrchr(full_path, '/');
	right_file_name = strrchr(full_path, '\\');

	if (left_file_name != NULL && right_file_name == NULL)
	{
		return left_file_name + 1;
	}
	else if (left_file_name == NULL && right_file_name != NULL)
	{
		return right_file_name + 1;
	}
	else if (left_file_name != NULL && right_file_name != NULL)
	{
		if (strlen(left_file_name) <= strlen(right_file_name))
		{
			return left_file_name + 1;
		}
		else
		{
			return right_file_name + 1;
		}
	}

	return full_path;
}

int initialize()
{
	if (profiler_dictionary == NULL)
	{
		profiler_dictionary = dictCreate(&dictTypePointerHash, NULL);
	}

	if (profiler_dictionary == NULL)
	{
		return -2;
	}

	if (breakpoint_dictionary == NULL)
	{
		breakpoint_dictionary = dictCreate(&dictTypeHeapIntCopyKey, NULL);
	}

	if (breakpoint_dictionary == NULL)
	{
		dictRelease(profiler_dictionary);
		profiler_dictionary = NULL;
		return -3;
	}

	memset(profiler_info_array, 0, sizeof(profiler_info_array));
	memset(tab, 0, sizeof(tab));

	//profiler_log_file = fopen("profiler.log", "w");

	return 0;
}

typedef union _key
{
	struct
	{
		unsigned char c1;
		unsigned char c2;
		unsigned char c3;
		unsigned char c4;
	} c;

	struct
	{
		unsigned short low;
		unsigned short high;
	} s;

	int k;
} key;

// 针对本项目特殊定制，key是文件名从第9-20字节+4字节的行号的，一共16字节
__forceinline const void* compute_key(lua_Debug* ar)
{
	unsigned long hash = 5381;
	key k;

	const char* c = (const char*)&ar->linedefined;
	const char* s = (const char*)&ar->source;
	k.k = (int) ar->source;
	hash = ((hash << 5) + hash) + c[0] + s[3]; /* hash * 33 + c */
	hash = ((hash << 5) + hash) + c[1] + s[2];
	k.s.high = (unsigned short)hash;
	return (const void*)k.k;
}

__forceinline void handle_return(lua_State* L, const lua_Debug* ar)
{
	long long currentTime = get_current_count();

	const void* key = compute_key(ar);

	profiler_info* info = dictFetchValue(profiler_dictionary, key);
	if (info != NULL)
	{
		if (info->source != ar->source || info->linedefined != ar->linedefined)
		{
			printf("hook_return\t%s:%d\t%s:%d\n", info->source, info->linedefined, ar->source, ar->linedefined);
		}
		else
		{
			long long interval = currentTime - info->last_call;
			if (interval > info->max)
			{
				info->max = interval;
			}
			info->total_with_children += interval;
			++(info->count);

			if (profiler_log_file != NULL && tab_amount < sizeof(tab))
			{
				--tab_amount;
				tab[tab_amount] = 0;
				fprintf(profiler_log_file, "%shook_return\t%s:%d\t%lld\t%lld\t%lld\n\n", tab, info->source, info->linedefined, info->last_call, info->total, info->total_with_children);
			}

			if (backtracking)
			{
				lua_Debug caller;
				size_t level = 1;
				info->total += interval;

				// 计算调用栈除去本函数后的调用时间
				while (lua_getstack(L, level, &caller) != 0 && lua_getinfo(L, "S", &caller) != 0)
				{
					if (ar->what[0] != 'L')
					{
						++level;
						continue;
					}

					key = compute_key(&caller);

					info  = NULL;
					info = dictFetchValue(profiler_dictionary, key);
					if (info != NULL)
					{
						if (info->source == caller.source && info->linedefined == caller.linedefined)
						{
							if (info->total > -99999999)
							{
								info->total -= interval;
							}
						}
						else
						{
							printf("compute_total\t%s:%d\t%s:%d\n", info->source, info->linedefined, caller.source, caller.linedefined);
						}
					}
					++level;
				}

			}
		}

	}

}


__forceinline void hook_call(lua_State* L, lua_Debug* ar)
{
	const void* key = NULL;
	profiler_info* info = NULL;

	if (current_profiler_position >= PROFILER_INFO_ARRAY_SIZE)
	{
		return;
	}

	if (lua_getinfo(L, "S", ar) == 0)
	{
		printf("hook_call getinfo error\t%s", ar->what);
		return;
	}

	if (ar->what[0] != 'L')
	{
		return;
	}

	key = compute_key(ar);

	info = dictFetchValue(profiler_dictionary, key);
	if (info == NULL)
	{
		info = &profiler_info_array[current_profiler_position];
		if (dictAdd(profiler_dictionary, key, info) != DICT_OK)
		{
			return;
		}

		++current_profiler_position;
		info->linedefined = ar->linedefined;
		info->source = ar->source;

		if (lua_getinfo(L, "n", ar) != 0 && ar->name != NULL)
		{
			memcpy(info->name, ar->name, sizeof(info->name) - 1);
			info->name[sizeof(info->name) - 1] = '\0';
		}

		info->last_call = get_current_count();
	}
	else
	{
		if (info->source != ar->source || info->linedefined != ar->linedefined)
		{
			printf("hook_call\t%s:%d\t%s:%d\n", info->source, info->linedefined, ar->source, ar->linedefined);
			return;
		}
		else
		{
			info->last_call = get_current_count();
		}
	}

	if (profiler_log_file != NULL && tab_amount < sizeof(tab) - 1)
	{
		fprintf(profiler_log_file, "%shook_call\t%s:%d\t%lld\t%lld\t%lld\n", tab, info->source, info->linedefined, info->last_call, info->total, info->total_with_children);
		tab[tab_amount] = ' ';
		++tab_amount;
	}
}

__forceinline void hook_return(lua_State* L, lua_Debug* ar)
{
	if (lua_getinfo(L, "S", ar) == 0)
	{
		return;
	}

	if (ar->what[0] != 'L')
	{
		return;
	}

	handle_return(L, ar);
}

int debugger_step_count = 0;
int debugger_next_count = 0;
int debugger_next_start_frame = 0;
int debugger_next_linedefined = 0;
char debugger_next_source[128] = { 0 };
lua_Debug last_breaked = {0};

__forceinline void do_break(lua_State* L, lua_Debug* ar)
{
	int n = lua_gettop(L);
	debugger_step_count = 0;
	debugger_next_count = 0;
	debugger_next_start_frame = 0;
	debugger_next_linedefined = 0;
	debugger_next_source[0] = '\0';
	memcpy(&last_breaked, ar, sizeof(last_breaked));

	lua_rawgeti(L, LUA_REGISTRYINDEX, break_callback);
	if (lua_type(L, -1) == LUA_TFUNCTION)
	{
		// 调用函数
		if (lua_pcall(L, lua_gettop(L) - 1 - n, LUA_MULTRET, 0))
		{
			printf("脚本执行错误，请检查脚本代码:%s\n", luaL_checkstring(L, -1));
		}
	}

	lua_pop(L, lua_gettop(L) - n);
}

__forceinline void hook_line(lua_State* L, lua_Debug* ar)
{
	dict* file_dictionary = NULL;

	if (L == NULL || ar == NULL)
	{
		return;
	}

	lua_getinfo(L, "Sn", ar);

	if (debugger_step_count > 0)
	{
		// 避免总在同一行打转
		if (last_breaked.currentline != ar->currentline || last_breaked.source != ar->source)
		{
			--debugger_step_count;
			if (debugger_step_count <= 0)
			{
				do_break(L, ar);
				return;
			}
		}
	}

	if (debugger_next_count > 0)
	{
		// 避免总在同一行打转
		if (last_breaked.currentline != ar->currentline || last_breaked.source != ar->source)
		{
			lua_Debug caller;
			int frame = debugger_next_start_frame;
			int ok = 0;
			int be_checked = 0;
			int be_called = 0;

			// 检查整个栈判断是否走到当前函数内部
			while ((ok = lua_getstack(L, frame, &caller)) == 1 && lua_getinfo(L, "S", &caller))
			{
				be_checked = 1;
				if (debugger_next_linedefined == caller.linedefined && strcmp(debugger_next_source, caller.source) == 0)
				{
					be_called = 1;
					break;
				}
				++frame;
			}

			// 此时ok == 0，表示自己已经是顶层调用，next应该被取消
			if (be_checked == 0)
			{
				debugger_next_count = 0;
			}
			else
			{
				if (be_called == 0)
				{
					--debugger_next_count;
					debugger_next_linedefined = ar->linedefined;
					strncpy(debugger_next_source, ar->source, sizeof(debugger_next_source) - 1);
				}

				if (debugger_next_count <= 0)
				{
					do_break(L, ar);
					return;
				}
			}
		}

	}

	file_dictionary = dictFetchValue(breakpoint_dictionary, &ar->currentline);
	if (file_dictionary != NULL)
	{
		int needBreak = 0;
		const char* file_name = trim_path(ar->source);
		breakpoint_info* bp = dictFetchValue(file_dictionary, file_name);
		if (bp == NULL)
		{
			return;
		}

		if (bp->type & breakpoint_type_condition)
		{
			int n = lua_gettop(L);

			lua_rawgeti(L, LUA_REGISTRYINDEX, bp->conditionFunction);
			if (lua_type(L, -1) == LUA_TFUNCTION)
			{
				// 测试条件是否为true
				if (lua_pcall(L, lua_gettop(L) - 1 - n, LUA_MULTRET, 0) == 0)
				{
					int resultAmount = lua_gettop(L) - n;
					if (resultAmount > 0 && lua_type(L, -resultAmount) == LUA_TBOOLEAN)
					{
						if (lua_toboolean(L, -resultAmount))
						{
							needBreak = 1;
						}
					}
				}
			}

			lua_pop(L, lua_gettop(L) - n);
		}
		else
		{
			needBreak = 1;
		}

		// 释放临时断点
		if (bp->type & breakpoint_type_temp)
		{
			dictDelete(file_dictionary, file_name);
			luaL_unref(L, LUA_REGISTRYINDEX, bp->conditionFunction);
			free(bp);
		}

		if (needBreak)
		{
			do_break(L, ar);
		}

	}
}

/* called by Lua (via the callhook mechanism) */
__forceinline void hook(lua_State* L, lua_Debug* ar)
{
	if (ar->event == LUA_HOOKCALL)
	{
		hook_call(L, ar);
	}
	else if (ar->event == LUA_HOOKRET)
	{
		hook_return(L, ar);
	}
	else if (ar->event == LUA_HOOKTAILRET)
	{
		hook_return(L, ar);
	}
	else if (ar->event == LUA_HOOKLINE)
	{
		hook_line(L, ar);
	}
}

__forceinline int debugger_start(lua_State* L)
{
	int hookmask = lua_gethookmask(L) | LUA_MASKLINE;
	lua_pushboolean(L, lua_sethook(L, hook, hookmask, 0));
	return 1;
}

__forceinline int debugger_stop(lua_State* L)
{
	int hookmask = lua_gethookmask(L) & ~LUA_MASKLINE;
	lua_pushboolean(L, lua_sethook(L, hook, hookmask, 0));
	return 1;
}

__forceinline int profiler_start(lua_State* L)
{
	int hookmask = lua_gethookmask(L) | LUA_MASKCALL | LUA_MASKRET;
	if (lua_isboolean(L, 1))
	{
		backtracking = lua_toboolean(L, 1);
	}
	lua_pushboolean(L, lua_sethook(L, hook, hookmask, 0));
	return 1;
}

__forceinline int profiler_stop(lua_State* L)
{
	int hookmask = lua_gethookmask(L) & ~LUA_MASKCALL & ~LUA_MASKRET;
	lua_pushboolean(L, lua_sethook(L, hook, hookmask, 0));
	return 1;
}

__forceinline int compare_profiler_info(const void* left, const void* right)
{
	long long difference = 0;
	if (backtracking)
	{
		difference = ((const profiler_info*)right)->total - ((const profiler_info*)left)->total;
	}
	else
	{
		difference = ((const profiler_info*)right)->total_with_children - ((const profiler_info*)left)->total_with_children;
	}
	return (int)(difference);// get_frequency());
}

int profiler_clear(lua_State* L)
{
	dictRelease(profiler_dictionary);
	profiler_dictionary = dictCreate(&dictTypePointerHash, NULL);

	memset(profiler_info_array, 0, sizeof(profiler_info_array));
	current_profiler_position = 0;

	memset(tab, 0, sizeof(tab));
	tab_amount = 0;

	lua_pushboolean(L, 1);
	return 1;
}

__forceinline int profiler_save(lua_State* L)
{
	static profiler_info infos[PROFILER_INFO_ARRAY_SIZE];
	size_t current_info_amount = 0;
	dictIterator* info_bucket_iterator = dictGetIterator(profiler_dictionary);
	dictEntry* info_bucket_entry = dictNext(info_bucket_iterator);
	for (; info_bucket_entry != NULL; info_bucket_entry = dictNext(info_bucket_iterator))
	{
		profiler_info* info = info_bucket_entry->val;
		if (info != NULL)
		{
			if (info != NULL && info->count > 0)
			{
				infos[current_info_amount] = *info;
				++current_info_amount;
				if (current_info_amount >= PROFILER_INFO_ARRAY_SIZE)
				{
					break;
				}
			}
		}
	}
	dictReleaseIterator(info_bucket_iterator);

	if (current_info_amount > 0)
	{
		long long frequency = get_frequency();
		FILE* file = NULL;
		if (lua_type(L, -1) == LUA_TSTRING)
		{
			wchar_t filename[260];
			if (MultiByteToWideChar(CP_UTF8, 0, lua_tostring(L, -1), -1, filename, sizeof(filename)) > 0)
			{
				file = _wfopen(filename, L"w");
				if (file != NULL)
				{
					if (fprintf(file, "%-40s\t%6s\t%-30s\t%10s\t%20s\t%10s\t%10s\t%10s\n", "[source]", "[line]", "[name]", "[total]", "[total with children]", "[average]", "[max]", "[count]") < 0)
					{
						fclose(file);
						file = NULL;
					}
				}
			}
		}

		qsort(infos, current_info_amount, PROFILER_INFO_SIZE, compare_profiler_info);

		{
			size_t i = 0;
			for (; i < current_info_amount; ++i)
			{
				profiler_info* info = &infos[i];
				if (file != NULL)
				{
					fprintf(file, "%-40s\t%6d\t%-30s\t%10lld\t%20lld\t%10lld\t%10lld\t%10u\n", info->source + 10, info->linedefined, info->name, info->total * 1000000 / frequency, info->total_with_children * 1000000 / frequency, info->total_with_children * 1000000 / (info->count * frequency), info->max * 1000000 / frequency, info->count);
				}
				else
				{
					printf("%s(%d):%s,total:[%lld],totalWithChildren:[%lld],average:[%lld],max:[%lld],count:[%u]\n", info->source + 10, info->linedefined, info->name, info->total * 1000000 / frequency, info->total_with_children * 1000000 / frequency, info->total_with_children * 1000000 / (info->count * frequency), info->max * 1000000 / frequency, info->count);
				}
			}
		}

		if (file != NULL)
		{
			fclose(file);
		}
	}

	dictPrintStats(profiler_dictionary);

	lua_pushboolean(L, 1);
	return 1;
}

__forceinline int debugger_register_break_callback(lua_State* L)
{
	if (lua_type(L, -1) == LUA_TFUNCTION)
	{
		if (break_callback != 0)
		{
			luaL_unref(L, LUA_REGISTRYINDEX, break_callback);
		}
		break_callback = luaL_ref(L, LUA_REGISTRYINDEX);

		lua_pushboolean(L, 1);
	}
	else
	{
		lua_pushboolean(L, 0);
	}
	return 1;
}

__forceinline int debugger_set_breakpoint(lua_State* L)
{
	if (lua_type(L, 1) == LUA_TNUMBER && lua_type(L, 2) == LUA_TSTRING)
	{
		dict* linedefined_dictionary = NULL;

		int line = luaL_checkint(L, 1);
		const char* source = luaL_checkstring(L, 2);
		const char* file_name = trim_path(source);
		if (file_name == NULL)
		{
			lua_pushinteger(L, 0);
			return 1;
		}

		linedefined_dictionary = dictFetchValue(breakpoint_dictionary, &line);
		if (linedefined_dictionary == NULL)
		{
			linedefined_dictionary = dictCreate(&dictTypeHeapStringCopyKey, NULL);
			if (linedefined_dictionary == NULL || dictAdd(breakpoint_dictionary, &line, linedefined_dictionary) != DICT_OK)
			{
				lua_pushinteger(L, 0);
				return 1;
			}
		}

		if (dictFetchValue(linedefined_dictionary, file_name) == NULL)
		{
			breakpoint_info* bp = malloc(sizeof(breakpoint_info));
			if (bp == NULL)
			{
				lua_pushinteger(L, 0);
				return 1;
			}

			if (dictAdd(linedefined_dictionary, file_name,  bp) != DICT_OK)
			{
				free(bp);
				lua_pushinteger(L, 0);
				return 1;
			}

			memset(bp, 0, sizeof(*bp));

			if (lua_type(L, 3) == LUA_TSTRING)
			{
				const char* condition = lua_tostring(L, 3);

				if (luaL_loadstring(L, condition) == 0)
				{
					bp->type |= breakpoint_type_condition;
					bp->conditionFunction = luaL_ref(L, LUA_REGISTRYINDEX);
				}
				else
				{
					free(bp);
					lua_pushinteger(L, 0);
					return 1;
				}

			}

			if (lua_type(L, 4) == LUA_TBOOLEAN)
			{
				if (lua_toboolean(L, 4))
				{
					bp->type |= breakpoint_type_temp;
				}
			}

			bp->line = line;
			strncpy(bp->file_name, file_name, sizeof(bp->file_name) - 1);
		}

		lua_pushinteger(L, 1);
	}
	else
	{
		lua_pushinteger(L, 0);
	}
	return 1;
}

__forceinline int debugger_clear_breakpoint(lua_State* L)
{
	if (lua_type(L, 1) == LUA_TNUMBER && lua_type(L, 2) == LUA_TSTRING)
	{
		dict* linedefined_dictionary = NULL;

		int line = luaL_checkint(L, 1);
		const char* source = luaL_checkstring(L, 2);
		const char* file_name = trim_path(source);
		if (file_name != NULL)
		{
			linedefined_dictionary = dictFetchValue(breakpoint_dictionary, &line);
			if (linedefined_dictionary != NULL)
			{
				breakpoint_info* bp = dictFetchValue(linedefined_dictionary, file_name);
				if (bp != NULL)
				{
					dictDelete(linedefined_dictionary, file_name);
					luaL_unref(L, LUA_REGISTRYINDEX, bp->conditionFunction);
					free(bp);
					lua_pushinteger(L, 1);
					return 1;
				}
			}
		}

	}
	lua_pushinteger(L, 0);
	return 1;
}

__forceinline int debugger_step(lua_State* L)
{
	if (lua_type(L, 1) == LUA_TNUMBER)
	{
		debugger_step_count = luaL_checkint(L, 1);
	}
	else
	{
		debugger_step_count = 1;
	}

	lua_pushboolean(L, 1);
	return 1;
}

__forceinline int debugger_next_implment(lua_State* L, int count, int linedefined, const char* source, int start_frame)
{
	debugger_next_count = count;
	debugger_next_linedefined = linedefined;
	strncpy(debugger_next_source, source, sizeof(debugger_next_source) - 1);
	debugger_next_start_frame = start_frame;

	lua_pushboolean(L, 1);
	return 1;
}

__forceinline int debugger_next(lua_State* L)
{
	if (lua_type(L, 1) == LUA_TNUMBER && lua_type(L, 2) == LUA_TNUMBER && lua_type(L, 3) == LUA_TSTRING)
	{
		return debugger_next_implment(L, luaL_checkint(L, 1), luaL_checkint(L, 2), lua_tostring(L, 3), 1);
	}

	lua_pushboolean(L, 0);
	return 1;

}

__forceinline int debugger_finish(lua_State* L)
{
	if (lua_type(L, 1) == LUA_TNUMBER && lua_type(L, 2) == LUA_TSTRING)
	{
		return debugger_next_implment(L, 1, luaL_checkint(L, 1), lua_tostring(L, 2), 0);
	}

	lua_pushboolean(L, 0);
	return 1;

}

static const luaL_reg prof_funcs[] =
{
	{ "Clear", profiler_clear },
	{ "Save", profiler_save },
	{ "Start", profiler_start },
	{ "Stop", profiler_stop },
	{ NULL, NULL }
};

static const luaL_reg debugger_funcs[] =
{
	{ "Start", debugger_start },
	{ "Stop", debugger_stop },
	{ "RegisterBreakCallback", debugger_register_break_callback },
	{ "SetBreakpoint", debugger_set_breakpoint },
	{ "ClearBreakpoint", debugger_clear_breakpoint },
	{ "Finish", debugger_finish },
	{ "Next", debugger_next },
	{ "Step", debugger_step },
	{ NULL, NULL }
};

static const luaL_reg check_funcs[] =
{
	{ "Start", checker_start },
	{ "Stop", checker_stop },
	{ NULL, NULL }
};

int luaopen_profiler(lua_State* L)
{
	if (initialize() != 0)
	{
		return 0;
	}

	luaL_register(L, "profiler", prof_funcs);
	luaL_register(L, "debug_helper", debugger_funcs);
	luaL_register(L, "checker", check_funcs);
	return 3;
}

int luaopen_profilerD(lua_State* L)
{
	return luaopen_profiler(L);
}
