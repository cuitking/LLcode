
BuildConsole "%~dp0\contrib\lua\luajit\luajit.sln" /prj=lua51 %*
@IF ERRORLEVEL 1 PAUSE

BuildConsole "%~dp0\contrib\luaProfiler\luaProfiler.sln" /prj=luaProfiler %*
@IF ERRORLEVEL 1 PAUSE

BuildConsole "%~dp0\contrib\luaJson\luaJson.sln" /prj=luaJson %*
@IF ERRORLEVEL 1 PAUSE

BuildConsole "%~dp0\contrib\mongo\mongoclient\mongoclient.sln" /prj=mongoclient %*
@IF ERRORLEVEL 1 PAUSE

BuildConsole "%~dp0\contrib\luaMongo\luaMongo.sln" /prj=luaMongo %*
@IF ERRORLEVEL 1 PAUSE
