#ifndef CHECKER_H_
#define CHECKER_H_

typedef struct lua_State lua_State;

int checker_start(lua_State* L);
int checker_stop(lua_State* L);

#endif