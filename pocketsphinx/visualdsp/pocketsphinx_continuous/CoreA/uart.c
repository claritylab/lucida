#include <drivers\adi_dev.h>				// device manager includes
#include <drivers\uart\adi_uart.h>				// uart driver includes
#include "uart.h"
#include <string.h>
#include "../ezkitutilities.h"
#include "dev.h"

#define OUTBUFLEN 256
#define INBUFLEN 256
#define min(a,b) ((a)<(b)?(a):(b))

// Create two buffer chains.  One chain will be used for one of the frames,
// the other chain for the other frame. Note that in this example there
// is only 1 buffer in each chain
ADI_DEV_1D_BUFFER InboundBuffer;
volatile ADI_DEV_1D_BUFFER OutboundBuffer;

u8 InputBuffer[INBUFLEN];
volatile u16 InputBufferhead, InputBuffertail;
// data for the inbound and outbound buffer
u8 InboundData;
u8 OutboundData[OUTBUFLEN];

volatile int sending_done = 1;

// Handle to the UART driver
static ADI_DEV_DEVICE_HANDLE 	DriverHandle;




/*********************************************************************

	Function:		Callback

	Description:	Each type of callback event has it's own unique ID 
					so we can use a single callback function for all 
					callback events.  The switch statement tells us 
					which event has occurred.
					
					In the example, we've configured the buffers going
					to the driver for inbound data to generate a callback
					but the buffers going to the driver for outbound 
					data will not generate a callback.  
					
					When we get a callback for an inbound buffer, we
					simply copy the data to the outbound buffer, then
					send the outbound buffer to the device for 
					transmission and re-queue the inbound buffer so that
					it can receive the next piece of data.
					
*********************************************************************/

static void Callback(
	void *AppHandle,
	u32  Event,
	void *pArg)
{
	
	ADI_DEV_1D_BUFFER *pBuffer;			// pointer to the buffer that was processed
	
	// CASEOF (event type)
	switch (Event) {
		
		// CASE (buffer processed)
		case ADI_DEV_EVENT_BUFFER_PROCESSED:
		
			// identify which buffer is generating the callback
			// we're setup to only get callbacks on inbound buffers so we know this 
			// is an inbound buffer
			pBuffer = (ADI_DEV_1D_BUFFER *)pArg;

			if (pArg == &InboundBuffer)
			{ 

		    	InputBuffer[InputBufferhead] = InboundData;
				InputBufferhead = (InputBufferhead +1) % INBUFLEN;
			    
				if ((InputBufferhead +1) % INBUFLEN != InputBuffertail)
				
				// requeue the inbound buffer
				ezErrorCheck(adi_dev_Read(DriverHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&InboundBuffer));
			}
			else if (pArg == &OutboundBuffer)
			{
			    sending_done = 1;
			}
			break;
			
		// CASE (an error)
		case ADI_UART_EVENT_BREAK_INTERRUPT:
			ezErrorCheck(ADI_UART_EVENT_BREAK_INTERRUPT);
			break;
		case ADI_UART_EVENT_FRAMING_ERROR:
			ezErrorCheck(ADI_UART_EVENT_FRAMING_ERROR);
			break;
		case ADI_UART_EVENT_PARITY_ERROR:
			ezErrorCheck(ADI_UART_EVENT_PARITY_ERROR);
			break;
		case ADI_UART_EVENT_OVERRUN_ERROR:
			ezErrorCheck(ADI_UART_EVENT_OVERRUN_ERROR);
			break;
			
	// ENDCASE
	}
	
	// return
}

u8 UARThit()
{
    return InputBufferhead != InputBuffertail;
}

u8 UARTsending()
{
    return !sending_done;
}

u8 UARTread()
{
	u8 data;
	
	while (!UARThit());
	
	data = InputBuffer[InputBuffertail];
	InputBuffertail = (InputBuffertail +1) % INBUFLEN;
	
	if (InboundBuffer.ProcessedFlag == 1)
	{
	    InboundBuffer.ProcessedFlag = 0;
	
		// requeue the inbound buffer
		ezErrorCheck(adi_dev_Read(DriverHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&InboundBuffer));
	}
		
	return data;
}

void UARTwrite(u8* data, u32 len)
{
    while (len>0)
    {
		while (UARTsending());
		sending_done = 0;
		
		OutboundBuffer.ElementCount = min(OUTBUFLEN,len);
		memcpy( OutboundData, data, OutboundBuffer.ElementCount );
		len -= OutboundBuffer.ElementCount;
		data+= OutboundBuffer.ElementCount;
		OutboundBuffer.ProcessedFlag = 0;
		
		// requeue the outbound buffer
		ezErrorCheck(adi_dev_Write(DriverHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&OutboundBuffer));
    }
	while (UARTsending());
}

void UARTinit( u32 baudrate )
{
    ADI_DCB_HANDLE			DCBManagerHandle = 0;		// handle to the callback service
	u32 cclk, sclk, vco;							// frequencies (note these are in MHz)
	u32 i; 											//loop variable
	
	ADI_DEV_CMD_VALUE_PAIR ConfigurationTable [] = {	// configuration table for the UART driver
		{ ADI_DEV_CMD_SET_DATAFLOW_METHOD, 	(void *)ADI_DEV_MODE_CHAINED	},
		{ ADI_UART_CMD_SET_DATA_BITS, 		(void *)8						},
		{ ADI_UART_CMD_ENABLE_PARITY, 		(void *)FALSE					},
		{ ADI_UART_CMD_SET_STOP_BITS, 		(void *)1						},
		{ ADI_UART_CMD_SET_BAUD_RATE, 		(void *)baudrate				},
		{ ADI_UART_CMD_SET_LINE_STATUS_EVENTS, 	(void *)TRUE					},
		{ ADI_DEV_CMD_END,					NULL							},
	};
	
	InputBufferhead = 0;
	InputBuffertail = 0;
	
	// create two buffers that will be initially be placed on the inbound queue
	// only the inbound buffer will have a callback
	InboundBuffer.Data = &InboundData;
	InboundBuffer.ElementCount = 1;
	InboundBuffer.ElementWidth = 1;
	InboundBuffer.CallbackParameter = (void*)&InboundBuffer;
	InboundBuffer.ProcessedFlag = FALSE;
	InboundBuffer.pNext = NULL;
	
	sending_done = 1;
	OutboundBuffer.Data = OutboundData;
	OutboundBuffer.ElementCount = 1;
	OutboundBuffer.ElementWidth = 1;
	OutboundBuffer.CallbackParameter = (void*)&OutboundBuffer;
	OutboundBuffer.ProcessedFlag = TRUE;
	OutboundBuffer.pNext = NULL;

	// open the UART driver for bidirectional data flow
	ezErrorCheck(adi_dev_Open(DeviceManagerHandle, &ADIUARTEntryPoint, 0, NULL, &DriverHandle, ADI_DEV_DIRECTION_BIDIRECTIONAL, NULL, DCBManagerHandle, Callback));
		
	// configure the UART driver with the values from the configuration table
   	ezErrorCheck(adi_dev_Control(DriverHandle, ADI_DEV_CMD_TABLE, ConfigurationTable));
		
	// give the device the inbound buffer
	ezErrorCheck(adi_dev_Read(DriverHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&InboundBuffer));

	// enable data flow
	ezErrorCheck(adi_dev_Control(DriverHandle, ADI_DEV_CMD_SET_DATAFLOW, (void *)TRUE));	
	
}
