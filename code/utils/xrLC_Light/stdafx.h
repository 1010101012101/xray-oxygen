// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#pragma once
#pragma warning (disable:4661)
#include "xrLC_Light.h"
#include "../xrInterface/xrThread.hpp"
#include "../xrInterface/LevelCompilerLoggerWindow.hpp"

extern ILevelCompilerLogger& Logger;
extern CThread::LogFunc ProxyMsg;
extern CThreadManager::ReportStatusFunc ProxyStatus;
extern CThreadManager::ReportProgressFunc ProxyProgress;
// TODO: reference additional headers your program requires here

#ifdef DEBUG
#	define CL_NET_LOG
#endif
