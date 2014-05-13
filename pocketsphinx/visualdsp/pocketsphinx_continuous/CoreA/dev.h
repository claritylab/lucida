#ifndef __DEV_H_
#define __DEV_H_

#include <drivers\adi_dev.h>				// device manager includes

extern ADI_DEV_MANAGER_HANDLE 	DeviceManagerHandle;	// handle to the Device Manager
extern ADI_DMA_MANAGER_HANDLE DMAHandle;				// DMAMgr handle
extern ADI_DMA_MANAGER_HANDLE DMAHandle;				// DMAMgr handle
extern ADI_DCB_HANDLE	Callback_Handle;

void devInit();

#endif
