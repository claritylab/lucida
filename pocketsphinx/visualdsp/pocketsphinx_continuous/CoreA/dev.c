#include "dev.h"				// device manager includes
#include "../ezkitutilities.h"

static u8 IntMgrData[(ADI_INT_SECONDARY_MEMORY * 2)];	// storage for interrupt manager
// Device Manager data (base memory + memory for 7 devices)
static u8 DevMgrData[ADI_DEV_BASE_MEMORY + (ADI_DEV_DEVICE_MEMORY * 7)];
static u8 DMAMgrData[ADI_DMA_BASE_MEMORY + (ADI_DMA_CHANNEL_MEMORY * 2)];	// storage for DMA manager
ADI_DEV_MANAGER_HANDLE 	DeviceManagerHandle = 0;	// handle to the Device Manager
ADI_DMA_MANAGER_HANDLE DMAHandle;				// DMAMgr handle
u32 ResponseCount;

// Interrupt to use for deferred callbacks (separate from the timer interrupts)
#define INTERRUPT_FOR_CALLBACK	12
#define QUEUE_SIZE		10

// storage for callback manager (only used for deferred callbacks)
ADI_DCB_HANDLE	Callback_Handle;
static u8		Callback_Manager_Storage[ADI_DCB_QUEUE_SIZE];
static u8		Callback_Queue[ADI_DCB_ENTRY_SIZE * QUEUE_SIZE];



/*********************************************************************

	Function:		ExceptionHandler
					HWErrorHandler

	Description:	We should never get an exception or hardware error, 
					but just in case we'll catch them and simply turn 
					on all the LEDS should one ever occur.

*********************************************************************/

static ADI_INT_HANDLER(ExceptionHandler)	// exception handler
{
		ezErrorCheck(1);
		return(ADI_INT_RESULT_PROCESSED);
}
		
	
static ADI_INT_HANDLER(HWErrorHandler)		// hardware error handler
{
		ezErrorCheck(1);
		return(ADI_INT_RESULT_PROCESSED);
}

void devInit()
{
    // initialize the Interrupt Manager and hook the exception and hardware error interrupts
	// all interrupts are on unique IVGs so we don't need any secondary handler memory
	ezErrorCheck(adi_int_Init(IntMgrData, sizeof(IntMgrData), &ResponseCount, NULL));
	
		// initialize callback manager and give it the storage area
	ezErrorCheck( adi_dcb_Init(Callback_Manager_Storage, sizeof(Callback_Manager_Storage), &ResponseCount, NULL));

	// initialize DMA
	ezErrorCheck(adi_dma_Init(	DMAMgrData,				// ptr to memory for use by DmaMgr
							sizeof(DMAMgrData),		// size of memory for use by DmaMgr
							&ResponseCount,			// response count
							&DMAHandle,				// ptr to DMA handle
							NULL));					// NULL

	//Initialize the flag service, memory is not passed because callbacks are not being used
	ezErrorCheck(adi_flag_Init(NULL, 0, &ResponseCount, NULL));	

	ezErrorCheck(adi_int_CECHook(3, ExceptionHandler, NULL, FALSE));
	ezErrorCheck(adi_int_CECHook(5, HWErrorHandler, NULL, FALSE));
	
	//Create a deferred callback queue, give it the storage for the queue, and receive the Handle for the queue
	ezErrorCheck( adi_dcb_Open(INTERRUPT_FOR_CALLBACK, Callback_Queue, sizeof(Callback_Queue), &ResponseCount, &Callback_Handle));	
	// initialize the Device Manager
	ezErrorCheck(adi_dev_Init(DevMgrData, sizeof(DevMgrData), &ResponseCount, &DeviceManagerHandle, NULL));
}