
PUSHD %cd%

SET CL_CONFIG="/D_SECURE_SCL_THROWS=1"

@CALL "%~dp0zBuildLua.bat" "/cfg=Debug-dll|Win32" /CL_ADD=%CL_CONFIG% %*

@CALL "%~dp0zBuildLibary.bat" "/cfg=Debug|Win32" /CL_ADD=%CL_CONFIG% %*

POPD