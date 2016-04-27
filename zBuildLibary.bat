CHCP 936

BuildConsole "%~dp0\contrib\hpf\hpf\hpf.vcproj" /prj=hpf %*
@IF ERRORLEVEL 1 PAUSE

BuildConsole "%~dp0\contrib\lz4\lz4.sln" /prj=lz4 %*
@IF ERRORLEVEL 1 PAUSE

BuildConsole "%~dp0\contrib\loki\loki-0.1.7\Loki.sln" /prj=Library %*
@IF ERRORLEVEL 1 PAUSE

BuildConsole "%~dp0\contrib\protobuf\vsprojects\protobuf.sln" /prj=libprotobuf %*
@IF ERRORLEVEL 1 PAUSE

BuildConsole "%~dp0\contrib\scgl\scgl.vcproj" /prj=scgl %*
@IF ERRORLEVEL 1 PAUSE

BuildConsole "%~dp0\contrib\log4cplus\log4cplus\msvc8\log4cplus.sln" /prj=log4cplus_dll %*
@IF ERRORLEVEL 1 PAUSE