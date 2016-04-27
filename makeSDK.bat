if exist .\version\nul goto NEXT
mkdir sdk

:NEXT

cd sdk
for /f %%a in ('dir /a:d /b') do rmdir /s /q %%a
del /s /q /f *.* 

xcopy /S /Y %~dp0\contrib\lib\*.lib lib\
xcopy /S /Y %~dp0\contrib\lib\*.dll dll\
xcopy /S /Y %~dp0\contrib\scgl\*.h include\
