#ifndef SCGL_WIN32_H
#define SCGL_WIN32_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		// 从 Windows 头中排除极少使用的资料
#endif

// 如果您必须使用下列所指定的平台之前的平台，则修改下面的定义。
// 有关不同平台的相应值的最新信息，请参考 MSDN。
#ifndef _WIN32_WINNT		    // 允许使用 Windows NT 4 或更高版本的特定功能。
#define _WIN32_WINNT 0x0502		//为 Windows Server 2003 SP1, Windows XP SP2 及更新版本改变为适当的值。
#endif

#include <windows.h>

#endif
