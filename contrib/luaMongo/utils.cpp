
#include "common.h"
#include "utils.h"

extern void push_bsontype_table(lua_State* L, mongo::BSONType bsontype);
void lua_push_value(lua_State* L, const mongo::BSONElement& elem);
const char* bson_name(int type);

static int mongo_itoa(int val, char* buf)
{
	const unsigned int radix = 10;

	char* p;
	unsigned int a;        //every digit
	int len;
	char* b;            //start of the digit char
	char temp;
	unsigned int u;

	p = buf;

	if (val < 0)
	{
		*p++ = '-';
		val = 0 - val;
	}
	u = (unsigned int)val;

	b = p;

	do
	{
		a = u % radix;
		u /= radix;

		*p++ = a + '0';

	} while (u > 0);

	len = (int)(p - buf);

	*p-- = 0;

	//swap
	do
	{
		temp = *p;
		*p = *b;
		*b = temp;
		--p;
		++b;

	} while (b < p);

	return len;
}

static int check_array(lua_State* L, int stackpos)
{
	int type = lua_type(L, stackpos);
	int maxindex = 0;

	if (type != LUA_TTABLE)
	{
		return -1;
	}

	int tablelen = lua_objlen(L, stackpos);

	lua_getglobal(L, "pairs");
	lua_pushvalue(L, stackpos - 1);
	lua_call(L, 1, 3);
	while (1)
	{
		lua_pushvalue(L, -3);
		lua_pushvalue(L, -3);
		lua_pushvalue(L, -3);
		lua_remove(L, -4);
		lua_call(L, 2, 2);
		if (lua_isnil(L, -2))
		{
			lua_pop(L, 4);
			break;
		}

		if (lua_isnumber(L, -2))
		{
			double index = lua_tonumber(L, -2);
			if (index == floor(index))
			{
				int indexint = (int)floor(index);
				maxindex = std::max(indexint, maxindex);
				lua_pop(L, 1);
			}
			else
			{
				lua_pop(L, 4);
				return -1;
			}
		}
		else
		{
			lua_pop(L, 4);
			return -1;
		}
	}

	if (tablelen != maxindex)
	{
		return -1;
	}

	return tablelen;
}

static void bson_to_array(lua_State* L, const mongo::BSONObj& obj)
{
	mongo::BSONObjIterator it = mongo::BSONObjIterator(obj);
	lua_newtable(L);
	int n = 1;
	while (it.more())
	{
		mongo::BSONElement elem = it.next();
		lua_push_value(L, elem);
		lua_rawseti(L, -2, n++);
	}
}

static void bson_to_table(lua_State* L, const mongo::BSONObj& obj)
{
	mongo::BSONObjIterator it = mongo::BSONObjIterator(obj);
	lua_newtable(L);//栈顶第三个-3
	while (it.more())
	{
		mongo::BSONElement elem = it.next();
		const char* key = elem.fieldName();
		lua_pushstring(L, key);//栈顶第二个-2
		lua_push_value(L, elem);//栈顶-1
		lua_rawset(L, -3);//pop key& value
	}
}

void lua_push_value(lua_State* L, const mongo::BSONElement& elem)
{
	int type = elem.type();

	switch (type)
	{
	case mongo::Undefined:
		lua_pushnil(L);
		break;
	case mongo::NumberInt:
		lua_pushnumber(L, elem.numberInt());
		break;
	case mongo::NumberLong:
		lua_pushinteger(L, elem.numberLong());
		break;
	case mongo::NumberDouble:
		lua_pushnumber(L, elem.number());
		break;
	case mongo::Bool:
		lua_pushboolean(L, elem.boolean());
		break;
	case mongo::String:
		lua_pushstring(L, elem.valuestr());
		break;
	case mongo::Array:
		bson_to_array(L, elem.embeddedObject());
		break;
	case mongo::Object:
		bson_to_table(L, elem.embeddedObject());
		break;
	case mongo::Date:
		push_bsontype_table(L, mongo::Date);
		lua_pushnumber(L, elem.date());
		lua_rawseti(L, -2, 1);
		break;
	case mongo::Timestamp:
		push_bsontype_table(L, mongo::Date);
		lua_pushnumber(L, elem.timestampTime());
		lua_rawseti(L, -2, 1);
		break;
	case mongo::Symbol:
		push_bsontype_table(L, mongo::Symbol);
		lua_pushstring(L, elem.valuestr());
		lua_rawseti(L, -2, 1);
		break;
	case mongo::RegEx:
		push_bsontype_table(L, mongo::RegEx);
		lua_pushstring(L, elem.regex());
		lua_rawseti(L, -2, 1);
		lua_pushstring(L, elem.regexFlags());
		lua_rawseti(L, -2, 2);
		break;
	case mongo::jstOID:
		push_bsontype_table(L, mongo::jstOID);
		lua_pushstring(L, elem.__oid().str().c_str());
		lua_rawseti(L, -2, 1);
		break;
	case mongo::jstNULL:
		//push_bsontype_table(L, mongo::jstNULL);
		lua_pushnil(L);
		break;
	case mongo::EOO:
		break;
	default:
		luaL_error(L, LUAMONGO_UNSUPPORTED_BSON_TYPE, bson_name(type));
	}
}

static void lua_append_bson(lua_State* L, const char* key, int stackpos, mongo::BSONObjBuilder* builder)
{
	int type = lua_type(L, stackpos);

	if (type == LUA_TTABLE)
	{
		int bsontype_found = luaL_getmetafield(L, stackpos, "__bsontype");
		if (!bsontype_found)
		{
			// not a special bsontype
			// handle as a regular table, iterating keys
			mongo::BSONObjBuilder b;

			int arraylen = check_array(L, stackpos);

			//2013.12.6 wangdebing 空表当作数组处理
			if (arraylen >= 0)
			{
				char buffer[16];
				for (int i = 0; i < arraylen; i++)
				{
					lua_rawgeti(L, stackpos, i + 1);
					mongo_itoa(i, buffer);
					lua_append_bson(L, buffer, -1, &b);
					lua_pop(L, 1);
				}

				builder->appendArray(key, b.obj());
			}
			else
			{
				lua_getglobal(L, "pairs");
				lua_pushvalue(L, stackpos - 1);
				lua_call(L, 1, 3);
				while (1)
				{
					lua_pushvalue(L, -3);
					lua_pushvalue(L, -3);
					lua_pushvalue(L, -3);
					lua_remove(L, -4);
					lua_call(L, 2, 2);
					if (lua_isnil(L, -2))
					{
						lua_pop(L, 4);
						break;
					}

					if (lua_isnumber(L, -2))
					{
						char buffer[16];
						int key = lua_tonumber(L, -2);
						mongo_itoa(key, buffer);
						lua_append_bson(L, buffer, -1, &b);
					}
					else
					{
						const char* k = lua_tostring(L, -2);
						lua_append_bson(L, k, -1, &b);
					}

					lua_pop(L, 1);
				}

				builder->append(key, b.obj());
			}
		}
		else
		{
			lua_Integer bson_type = lua_tointeger(L, -1);
			lua_pop(L, 1);
			lua_rawgeti(L, -1, 1);
			switch (bson_type)
			{
			case mongo::Date:
				builder->appendDate(key, lua_tonumber(L, -1));
				break;
			case mongo::Timestamp:
				builder->appendTimestamp(key);
				break;
			case mongo::RegEx:
			{
				const char* regex = lua_tostring(L, -1);
				lua_rawgeti(L, -2, 2); // options
				const char* options = lua_tostring(L, -1);
				lua_pop(L, 1);
				builder->appendRegex(key, regex, options);
				break;
			}
			case mongo::NumberInt:
				builder->append(key, static_cast<mongo::uint32_t>(lua_tonumber(L, -1)));
				break;
			case mongo::NumberLong:
				builder->append(key, lua_tointeger(L, -1));
				break;
			case mongo::NumberDouble:
				builder->append(key, lua_tonumber(L, -1));
				break;
			case mongo::Symbol:
				builder->appendSymbol(key, lua_tostring(L, -1));
				break;
			case mongo::jstOID:
			{
				mongo::OID oid;
				oid.init(lua_tostring(L, -1));
				builder->appendOID(key, &oid);
				break;
			}
			case mongo::jstNULL:
				builder->appendNull(key);
				break;

			default:
				luaL_error(L, LUAMONGO_UNSUPPORTED_BSON_TYPE, luaL_typename(L, stackpos));
			}
			lua_pop(L, 1);
		}
	}
	else if (type == LUA_TNIL)
	{
		builder->appendNull(key);
	}
	else if (type == LUA_TNUMBER)
	{
		lua_Number numval = luaL_checknumber(L, stackpos);
		builder->append(key, numval);
	}
	else if (type == LUA_TBOOLEAN)
	{
		builder->appendBool(key, lua_toboolean(L, stackpos) != 0);
	}
	else if (type == LUA_TSTRING)
	{
		builder->append(key, lua_tostring(L, stackpos));
	}
	else
	{
		luaL_error(L, LUAMONGO_UNSUPPORTED_LUA_TYPE, luaL_typename(L, stackpos));
	}
}

void bson_to_lua(lua_State* L, const mongo::BSONObj& obj)
{
	if (obj.isEmpty())
	{
		lua_pushnil(L);
	}
	else
	{
		bson_to_table(L, obj);
	}
}

// stackpos must be relative to the bottom, i.e., not negative
void lua_to_bson(lua_State* L, int stackpos, mongo::BSONObj& obj)
{
	mongo::BSONObjBuilder builder;

	lua_getglobal(L, "pairs");
	lua_pushvalue(L, stackpos);
	lua_call(L, 1, 3);
	while (1)
	{
		lua_pushvalue(L, -3);
		lua_pushvalue(L, -3);
		lua_pushvalue(L, -3);
		lua_remove(L, -4);
		lua_call(L, 2, 2);
		if (lua_isnil(L, -2))
		{
			lua_pop(L, 4);
			break;
		}

		if (lua_isnumber(L, -2))
		{
			char buffer[16];
			int key = lua_tonumber(L, -2);
			mongo_itoa(key, buffer);
			lua_append_bson(L, buffer, -1, &builder);
		}
		else
		{
			const char* k = lua_tostring(L, -2);
			lua_append_bson(L, k, -1, &builder);
		}

		lua_pop(L, 1);
	}

	obj = builder.obj();
}

const char* bson_name(int type)
{
	const char* name;

	switch (type)
	{
	case mongo::EOO:
		name = "EndOfObject";
		break;
	case mongo::NumberDouble:
		name = "NumberDouble";
		break;
	case mongo::String:
		name = "String";
		break;
	case mongo::Object:
		name = "Object";
		break;
	case mongo::Array:
		name = "Array";
		break;
	case mongo::BinData:
		name = "BinData";
		break;
	case mongo::Undefined:
		name = "Undefined";
		break;
	case mongo::jstOID:
		name = "ObjectID";
		break;
	case mongo::Bool:
		name = "Bool";
		break;
	case mongo::Date:
		name = "Date";
		break;
	case mongo::jstNULL:
		name = "NULL";
		break;
	case mongo::RegEx:
		name = "RegEx";
		break;
	case mongo::DBRef:
		name = "DBRef";
		break;
	case mongo::Code:
		name = "Code";
		break;
	case mongo::Symbol:
		name = "Symbol";
		break;
	case mongo::CodeWScope:
		name = "CodeWScope";
		break;
	case mongo::NumberInt:
		name = "NumberInt";
		break;
	case mongo::Timestamp:
		name = "Timestamp";
		break;
	case mongo::NumberLong:
		name = "NumberLong";
		break;
	default:
		name = "UnknownType";
	}

	return name;
}
