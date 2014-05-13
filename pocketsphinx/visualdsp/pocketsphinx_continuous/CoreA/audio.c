#include "dev.h"
#include <drivers\codec\adi_ad1836a_ii.h>				// uart driver includes
#include "audio.h"
#include <string.h>
#include <sys/exception.h>
#include <cdefBF561.h>
#include <ccblkfn.h>
#include <sysreg.h>
#include "ezkitutilities.h"

#define OUTBUFLEN 256
#define INBUFLEN 256
#define min(a,b) ((a)<(b)?(a):(b))

// define the structure to access table of device registers (register address, register configuration value) 
ADI_DEV_ACCESS_REGISTER Cfg_Regs [ ] = {
					{AD1836A_DAC_CTRL_1, 0x000},
					{AD1836A_DAC_CTRL_2, 0x000},
					{AD1836A_DAC_1L_VOL, 0x3ff},
					{AD1836A_DAC_1R_VOL, 0x3ff},
					{AD1836A_DAC_2L_VOL, 0x3ff},
					{AD1836A_DAC_2R_VOL, 0x3ff},
					{AD1836A_DAC_3L_VOL, 0x3ff},
					{AD1836A_DAC_3R_VOL, 0x3ff},
					{AD1836A_ADC_CTRL_1, 0x000},
					{AD1836A_ADC_CTRL_2, 0x180},
					{AD1836A_ADC_CTRL_3, 0x000},
					{ADI_DEV_REGEND    , 0    }}; 
 
// Create two buffer chains.  One chain will be used for one of the frames,
// the other chain for the other frame. Note that in this example there
// is only 1 buffer in each chain
static ADI_DEV_1D_BUFFER InboundBuffer;
static volatile ADI_DEV_1D_BUFFER OutboundBuffer;

static u8 InboundData[INBUFLEN*32];
static u8 OutboundData[OUTBUFLEN*32];

// Handle to the UART driver
static ADI_DEV_DEVICE_HANDLE 	AudioDriverHandle;

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

static void AudioCallback(
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
			
			if (pBuffer == &InboundBuffer )
			{
				//memcpy(OutboundData,InboundData,4*8*INBUFLEN);
				// give the device the inbound buffer
				ezErrorCheck(adi_dev_Read( AudioDriverHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&InboundBuffer) );				
			}

			if (pBuffer == &OutboundBuffer)
				// give the device the outbound buffer
				ezErrorCheck(adi_dev_Write( AudioDriverHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&OutboundBuffer) );

			break;
			
	// ENDCASE
	}
	
	// return
}

void audioInit()
{
	u32 cclk, sclk, vco;							// frequencies (note these are in MHz)
	u32 i; 											//loop variable
	

	// create two buffers that will be initially be placed on the inbound queue
	// only the inbound buffer will have a callback
	InboundBuffer.Data = InboundData;
	InboundBuffer.ElementCount = INBUFLEN*8;
	InboundBuffer.ElementWidth = 4;
	InboundBuffer.CallbackParameter = (void*)&InboundBuffer;
	InboundBuffer.ProcessedFlag = FALSE;
	InboundBuffer.ProcessedElementCount = 0;
	InboundBuffer.pNext = NULL;
	
	OutboundBuffer.Data = InboundData;
	OutboundBuffer.ElementCount = OUTBUFLEN*8;
	OutboundBuffer.ElementWidth = 4;
	OutboundBuffer.CallbackParameter = (void*)&OutboundBuffer;
	OutboundBuffer.ProcessedFlag = FALSE;
	OutboundBuffer.ProcessedElementCount = 0;
	OutboundBuffer.pNext = NULL;

	// open the audio driver for bidirectional data flow
	//ezErrorCheck(adi_dev_Open(DeviceManagerHandle, &ADIAD1836AEntryPoint, 0, (void*)0x1836, &AudioDriverHandle, ADI_DEV_DIRECTION_BIDIRECTIONAL, DMAHandle, DCBManagerHandle, AudioCallback));
	ezErrorCheck(adi_dev_Open(DeviceManagerHandle, &ADIAD1836AEntryPoint, 0, (void*)0x1836, &AudioDriverHandle, ADI_DEV_DIRECTION_BIDIRECTIONAL, DMAHandle, NULL, AudioCallback));
		
	ezErrorCheck(adi_dev_Control(AudioDriverHandle, ADI_AD1836A_CMD_SET_SPORT_DEVICE_NUMBER, (void *) 0));
	ezErrorCheck(adi_dev_Control(AudioDriverHandle, ADI_AD1836A_CMD_SET_SPORT_STATUS,(void *) ADI_AD1836A_SPORT_OPEN));  
	
	ezErrorCheck(adi_dev_Control(AudioDriverHandle, ADI_AD1836A_CMD_SET_SPI_CS, (void *) 4));  

	// Pull AD1836A from reset mode 
	// PF15 is connected to reset pin of AD1836A 
	adi_flag_Open(ADI_FLAG_PF15); 
	// set this flag pin as output 
	adi_flag_SetDirection(ADI_FLAG_PF15, ADI_FLAG_DIRECTION_OUTPUT); 
	// clear this pin output to reset AD1836A 
	adi_flag_Clear(ADI_FLAG_PF15); 
	//wait at least 5 ns in reset 
	asm("nop; nop; nop; nop; nop;"); 
	// tie it to high again to enable AD1836A 
	adi_flag_Set(ADI_FLAG_PF15); 	
	
    // Registers listed in the table will be configured with corresponding table Data values 
	ezErrorCheck(adi_dev_Control(AudioDriverHandle, ADI_DEV_CMD_REGISTER_TABLE_WRITE, (void *) & Cfg_Regs [0]));
	ezErrorCheck(adi_dev_Control(AudioDriverHandle, ADI_DEV_CMD_SET_DATAFLOW_METHOD, (void*)ADI_DEV_MODE_CHAINED));
	
	// give the device the inbound buffer
	ezErrorCheck(adi_dev_Read( AudioDriverHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&InboundBuffer) );
	// give the device the outbound buffer
	ezErrorCheck(adi_dev_Write( AudioDriverHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&OutboundBuffer) );
		
	ezErrorCheck(adi_dev_Control(AudioDriverHandle, ADI_DEV_CMD_SET_DATAFLOW, (void*)TRUE));
}
