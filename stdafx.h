
// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件

#pragma once

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN // 从 Windows 头中排除极少使用的资料
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS // 某些 CString 构造函数将是显式的

// 关闭 MFC 对某些常见但经常可放心忽略的警告消息的隐藏
#define _AFX_ALL_WARNINGS

#include <afxwin.h> // MFC 核心组件和标准组件
#include <afxext.h> // MFC 扩展

#include <afxdisp.h> // MFC 自动化类

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h> // MFC 对 Internet Explorer 4 公共控件的支持
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h> // MFC 对 Windows 公共控件的支持
#endif              // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h> // 功能区和控件条的 MFC 支持

#define WM_SOCKET WM_USER + 220
#define WM_MODBUSSVR WM_USER + 221
#define PORT 502
#define PIXEL_FLAG 2051
#define PIXEL 2048
#define SERIALRECV_LEN 100
#define SERIALSEND_LEN 100

//#define LOGFILEPATH		"F:\\VcProj\\SvrSytem\\Log\\%s.log"
//#define DATAFILEPATH	"F:\\VcProj\\SvrSytem\\Data\\%s.csv"
//#define DATAPATYMD		"F:\\VcProj\\SvrSytem\\Data\\%04d%02d%02d*.csv"
//#define DATAFILEPATH2	"F:\\VcProj\\SvrSytem\\Data\\%s"
//#define RECFILEPATH		"F:\\VcProj\\SvrSytem\\Log\\Rec%s.txt"
//#define CONFIGFILE		"F:\\VcProj\\SvrSytem\\Data\\Config.ini"

#define LOGFILEPATH "..\\Log\\%s.log"
#define DATAFILEPATH "..\\Data\\%s.csv"
#define DATAPATYMD "..\\Data\\%04d%02d%02d*.csv"
#define DATAFILEPATH2 "..\\Data\\%s"
#define RECFILEPATH "..\\Log\\Rec%s.txt"
#define CONFIGFILE "..\\Data\\Config.ini"

#define PORT 502

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif
