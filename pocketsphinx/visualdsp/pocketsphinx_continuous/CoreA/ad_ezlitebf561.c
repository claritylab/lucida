/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2001 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */

/*
 * ad.c -- Wraps a "sphinx-II standard" audio interface around the basic audio
 * 		utilities.
 *
 * HISTORY
 * 
 * 11-Jun-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Modified to conform to new A/D API.
 * 
 * 12-May-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Dummy template created.
 */

#include <stdio.h>
#include <string.h>
#include <config.h>
#include <sys\exception.h>
#include <cdefBF561.h>
#include <ccblkfn.h>
#include <sysreg.h>
#include <math.h>
#include <drivers\adi_dev.h>				// device manager includes
#include <drivers\sport\adi_sport.h>				// uart driver includes
#include "dev.h"
#include <drivers\codec\adi_ad1836a_ii.h>				// uart driver includes

#include "ckd_alloc.h"
#include "audio.h"
#include "prim_type.h"
#include "ad.h"
#include "err.h"
#include "../ezkitutilities.h"

volatile int* iRxBuffer1;
volatile int* iRxBuffer2;

// define the structure to access table of device registers (register address, register configuration value) 
ADI_DEV_ACCESS_REGISTER Cfg_Regs [ ] = 
{
	{AD1836A_DAC_CTRL_1, 0x000},
	{AD1836A_DAC_CTRL_2, 0x000},
	{AD1836A_DAC_1L_VOL, 0x3ff},
	{AD1836A_DAC_1R_VOL, 0x3ff},
	{AD1836A_DAC_2L_VOL, 0x3ff},
	{AD1836A_DAC_2R_VOL, 0x3ff},
	{AD1836A_DAC_3L_VOL, 0x3ff},
	{AD1836A_DAC_3R_VOL, 0x3ff},
	{AD1836A_ADC_CTRL_1, 0x100},
	{AD1836A_ADC_CTRL_2, 0x180},
	{AD1836A_ADC_CTRL_3, 0x000},
	{ADI_DEV_REGEND    , 0    }
}; 

ADI_DEV_1D_BUFFER AudioInboundBuffer;
ADI_DEV_1D_BUFFER AudioInboundBuffer2;
ADI_DEV_1D_BUFFER AudioOutboundBuffer;
ADI_DEV_1D_BUFFER AudioOutboundBuffer2;
// Handle to the UART driver
static ADI_DEV_DEVICE_HANDLE 	AudioDriverHandle;
int iStoreAudio;
// simple ring buffer
int16* bigRecBuf;
volatile int head,tail;

static int sampling_divisor=-1;

void Process_Data(ADI_DEV_1D_BUFFER* buf);

static void CallbackRoutine(
	void *ClientHandle,
	u32  Event,
	void *pArg)
{
	Process_Data( (ADI_DEV_1D_BUFFER*)pArg );
}



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

			if (pArg == &AudioInboundBuffer)
			{
			    
			    //Process_Data( (ADI_DEV_1D_BUFFER*)pArg );
				// Post a deferred callback
				adi_dcb_Post(
					Callback_Handle,	// The handle of the required queue server
					1,					// The software priority of the entry
					CallbackRoutine,	// The address of the callback function
					(void *) 0,			// Unused client argument
					0,					// Unused client argument
					(void *) &AudioInboundBuffer);		// Unused client argument
			    
			}
			else if (pArg == &AudioInboundBuffer2)
			{ 
			    				// Post a deferred callback
			    //Process_Data( (ADI_DEV_1D_BUFFER*)pArg );
				adi_dcb_Post(
					Callback_Handle,	// The handle of the required queue server
					1,					// The software priority of the entry
					CallbackRoutine,	// The address of the callback function
					(void *) 0,			// Unused client argument
					0,					// Unused client argument
					(void *) &AudioInboundBuffer2);		// Unused client argument
					
			}
			else if (pArg == &AudioOutboundBuffer)
			{
			}
			else if (pArg == &AudioOutboundBuffer2)
			{
			}
			break;
			
		// CASE (an error)
		case ADI_SPORT_EVENT_ERROR_INTERRUPT:
			ezErrorCheck(ADI_SPORT_EVENT_ERROR_INTERRUPT);
			break;
			
	// ENDCASE
	}
	
	// return
}

void audioInit()
{
//	u32 cclk, sclk, vco;							// frequencies (note these are in MHz)
//	u32 i; 											//loop variable
	u32 ResponseCount;
	
	const int elwidth = 4;
	const int elcount = ADDMABUFLEN*(32/elwidth);	
	
	iRxBuffer1= ckd_malloc(sizeof(int)*ADDMABUFLEN*8);
	iRxBuffer2= ckd_malloc(sizeof(int)*ADDMABUFLEN*8);
	bigRecBuf= ckd_malloc(sizeof(int16)*ADBUFSIZE);
		// create two buffers that will be initially be placed on the inbound queue
	// only the inbound buffer will have a callback
	AudioInboundBuffer.Data = (void*)iRxBuffer1;
	AudioInboundBuffer.ElementCount = elcount;
	AudioInboundBuffer.ElementWidth = elwidth;
	AudioInboundBuffer.CallbackParameter = (void*)&AudioInboundBuffer;
	AudioInboundBuffer.ProcessedFlag = FALSE;
	AudioInboundBuffer.ProcessedElementCount = 0;
	AudioInboundBuffer.pNext = &AudioInboundBuffer2;
	
	AudioInboundBuffer2.Data = (void*)iRxBuffer2;
	AudioInboundBuffer2.ElementCount = elcount;
	AudioInboundBuffer2.ElementWidth = elwidth;
	AudioInboundBuffer2.CallbackParameter = (void*)&AudioInboundBuffer2;
	AudioInboundBuffer2.ProcessedFlag = FALSE;
	AudioInboundBuffer2.ProcessedElementCount = 0;
	AudioInboundBuffer2.pNext = NULL;	
	
	AudioOutboundBuffer.Data = (void*)iRxBuffer1;
	AudioOutboundBuffer.ElementCount = elcount;
	AudioOutboundBuffer.ElementWidth = elwidth;
	AudioOutboundBuffer.CallbackParameter = (void*)&AudioOutboundBuffer;
	AudioOutboundBuffer.ProcessedFlag = FALSE;
	AudioOutboundBuffer.ProcessedElementCount = 0;
	AudioOutboundBuffer.pNext = &AudioOutboundBuffer2;
	
	AudioOutboundBuffer2.Data = (void*)iRxBuffer2;
	AudioOutboundBuffer2.ElementCount = elcount;
	AudioOutboundBuffer2.ElementWidth = elwidth;
	AudioOutboundBuffer2.CallbackParameter = (void*)&AudioOutboundBuffer2;
	AudioOutboundBuffer2.ProcessedFlag = FALSE;
	AudioOutboundBuffer2.ProcessedElementCount = 0;
	AudioOutboundBuffer2.pNext = NULL;	


	// open the audio driver for bidirectional data flow
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
	ezErrorCheck(adi_dev_Control(AudioDriverHandle, ADI_DEV_CMD_SET_DATAFLOW_METHOD, (void*)ADI_DEV_MODE_CHAINED_LOOPBACK));
}

void Process_Data(ADI_DEV_1D_BUFFER* buf)
{
    int scnt,cnt,idx,avg=0;
    unsigned int val;
    signed int* data = buf->Data;
    
    if (!buf->ProcessedFlag)
    	return;
    
    for (cnt=0,scnt=1,idx=INTERNAL_ADC_L0;cnt<ADDMABUFLEN;cnt++,idx+=8,scnt++)
    {		
        //val = ((unsigned int*)(buf->Data))[idx];
        //val <<= 1;
	    //avg+=(*(int*)(&val))>>16;
	    avg+=data[idx]>>16;
	    
		if (scnt >= sampling_divisor) 
		{
			scnt = 0;
			avg /= sampling_divisor;
		
			if (((head+1) % ADBUFSIZE) == tail)
			{
				ezToggleAllLEDs();
				return;
			}
			
			// TODO: LP Filter!
			//printf("%d -> %d\n",val,avg);
			bigRecBuf[ head++ ] = avg;
			
			//bigRecBuf[ head++ ] = data[idx]>>16;
			avg = 0;
			if (head>=ADBUFSIZE)
			{
				head = 0;
			}
		}
    }
    buf->ProcessedFlag = FALSE;
}

ad_rec_t *
ad_open_dev(const char *dev, int32 samples_per_sec)
{
    fprintf(stderr, "A/D library not implemented\n");
    return NULL;
}

ad_rec_t *
ad_open_sps(int32 samples_per_sec)
{
    ad_rec_t *adrec;
    int a;
    sampling_divisor=-1;
    
    for (a=1;a<16;a++)
    	if (samples_per_sec == 48000/a)
    	{
    		sampling_divisor = a;
    		break;
    	}
    
    if (sampling_divisor<0)
    	E_FATAL("this audio driver supports only certain things.");
    
	adrec = (ad_rec_t *)ckd_calloc(1,sizeof(ad_rec_t));
	adrec->sps = samples_per_sec;
	adrec->bps = 2; // 16bit
	iStoreAudio = 0;
	
	audioInit();
	
	return adrec;
}

ad_rec_t *
ad_open(void)
{
    
    return ad_open_sps(DEFAULT_SAMPLES_PER_SEC);
}


int32
ad_start_rec(ad_rec_t * r)
{
    iStoreAudio = 1;
	// give the device the inbound buffer
	ezErrorCheck(adi_dev_Read( AudioDriverHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&AudioInboundBuffer) );
	// give the device the outbound buffer
	ezErrorCheck(adi_dev_Write( AudioDriverHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&AudioOutboundBuffer) );
   	ezErrorCheck(adi_dev_Control(AudioDriverHandle, ADI_DEV_CMD_SET_DATAFLOW, (void*)TRUE));
    return 0;
}


int32
ad_stop_rec(ad_rec_t * r)
{
   	ezErrorCheck(adi_dev_Control(AudioDriverHandle, ADI_DEV_CMD_SET_DATAFLOW, (void*)FALSE));
    iStoreAudio = 0;
    return 0;
}


int32
ad_read(ad_rec_t * r, int16 * buf, int32 max)
{
    int len;
    
    if (head == tail && iStoreAudio == 1)
    	return 0;

    if (head == tail && iStoreAudio == 0)
    	return -1;
    	
    	
    if (tail > head)
    {
        len = min(ADBUFSIZE-tail,max);
    }
    else
    {
        len = min(head-tail,max);
    }
    
	memcpy(buf,(const void*)(bigRecBuf+tail),len*sizeof(int16));
	tail+=len;
	if (tail>=ADBUFSIZE)
		tail = 0;
	    	
    return len;
}


int32
ad_close(ad_rec_t * r)
{
    ckd_free(r);
    return 0;
}

