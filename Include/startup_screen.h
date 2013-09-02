#ifndef __STARTUP_SCREEN_H_
#define	__STARTUP_SCREEN_H_

#include "Util.h";

ret_code startupScreenInit();

#ifdef _CUTEST
#include "CuTest.h"
CuSuite* CuGetStartupScreenSuite(void);
#endif

#endif	// __STARTUP_SCREEN_H_


