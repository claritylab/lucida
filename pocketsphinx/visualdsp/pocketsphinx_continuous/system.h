#ifndef  _SYSTEM_DEFINED
#define _SYSTEM_DEFINED

#include "cdefBF561.h"
#include "ccblkfn.h"
#include <sys\exception.h>


// #define TC_PER 			0xFFC00B0C
// #define pTC_PER 		(volatile unsigned short *)TC_PER
	

/********************************************************************************/
/***** Symbolic constants													*****/
/********************************************************************************/

// system constants  

#define CLKIN	  (30.0e6)		// clockin frequency in Hz
#define CORECLK  (600.0e6)		// core clock frequency in Hz
#define SYSCLK	  (120.0e6)		// system clock frequency in Hz

void Set_PLL(short CoreCLOCK_multiplier, short SCLK_divider);


#endif

