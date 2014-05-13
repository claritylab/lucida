/*
   This file is part of the imp project.
   Copyright (C) 2009 Università degli Studi di Bergamo, Politecnico di Milano
   Authors:
       Cristian Gatti, gatti DOT kris AT gmail DOT com
       Silvio Moioli, silvio AT moioli DOT net, <http://www.moioli.net>
 */

#include "config.h"

#if defined(AD_BACKEND_S60)

/*
    S60 Sphinx audio backend.
    Currently it is limited to recording 8kHz PCM16 mono audio data.
 */

//Symbian includes must go first
#include <e32base.h>
#include <e32msgqueue.h>
#include <e32debug.h>
#include <MdaAudioInputStream.h>
#include <mda/common/audio.h>

#include "ad.h"

/* 
 * Implementation notes
 * Since Symbian uses a callback system based on Active Objects to carry out asynchronous
 * operations we must make use of a helper thread, which is also useful for priority reasons.
 * 
 * Sphinxbase functions are implemented through the CAudioDevice class that communicates
 * with the helper thread, which is encapsulated in CHelperThreadHost. Threads use:
 *  - a synchronized temporaryBuffer and
 *  - Symbian thread-safe queues (RMsgQueues)
 * to communicate.
 */

//constants

/* 
 * Messages sent through RMsgQueues.
 */
enum TMessage {
    ENullMessage = 0,
    EInited,
    EStartRecording,
    ERecordingStarted,
    EStopRecording,
    ERecordingStopped,
    EClose,
    EClosed
};

/* 
 * Max RMsgQueue size (will block if full).
 */
const TInt KQueueLength = 10;

/* 
 * Only PCM16 is supported at the moment.
 */
const TInt KBytesPerSample = 2;

/* 
 * Only 16kHz audio is supported at the moment.
 */
const TInt KSampleRate = 16000;

/* 
 * Temporary buffer length in milliseconds. The temporary buffer is filled
 * by the OS and then copied to the main buffer where it is read by Sphinxbase
 * functions.
 */
const TInt KTemporaryBufferTime = 150;

/* 
 * Temporary buffer length in bytes.
 */
const TInt KTemporaryBufferSize = (KTemporaryBufferTime * KSampleRate * KBytesPerSample) / 1000;

/* 
 * Helper thread name.
 */
_LIT(KHelperThreadName, "HelperThread");

/* 
 * Possible helper thread states.
 */
enum THelperThreadState {EPaused = 0, ERecording, EClosing};

//classes

/* 
 * Helper thread wrapper class.
 */
class CHelperThreadHost : public MMdaAudioInputStreamCallback {
    public:
        CHelperThreadHost(CBufSeg*, RFastLock*, RMsgQueue<TInt>*, RMsgQueue<TInt>*);
        virtual ~CHelperThreadHost();
        static TInt ThreadFunction(TAny*);
        void InitializeL();
        void DestroyL();
        
        virtual void MaiscOpenComplete(TInt);
        virtual void MaiscBufferCopied(TInt, const TDesC8&);
        virtual void MaiscRecordComplete(TInt);
        
    private:
        CMdaAudioInputStream* iStream;
        TMdaAudioDataSettings iStreamSettings;
        THelperThreadState iState;

        RBuf8 iTemporaryBuffer;

        CBufSeg* iBuffer;
        RFastLock* iBufferLock;

        RMsgQueue<TInt>* iCommandQueue;
        RMsgQueue<TInt>* iNotificationQueue;
};

/* 
 * Class used to invoke Symbian functions from Sphinx functions.
 */
class CAudioDevice {
    public:
        CAudioDevice();
        void ConstructL();
        static CAudioDevice* NewL();
        virtual ~CAudioDevice();
        
        void ResumeRecording();
        void PauseRecording();
        TInt ReadSamples(TAny*, TInt);
        
    private:
        RThread iThread;
        CHelperThreadHost* iThreadHost;
        
        CBufSeg* iBuffer;
        RFastLock iBufferLock;
        
        RMsgQueue<TInt> iCommandQueue;
        RMsgQueue<TInt> iNotificationQueue;
};

CAudioDevice::CAudioDevice(){
    iCommandQueue.CreateLocal(KQueueLength);
    iNotificationQueue.CreateLocal(KQueueLength);
}

void CAudioDevice::ConstructL(){
    iBuffer = CBufSeg::NewL(KTemporaryBufferSize);
    iBufferLock.CreateLocal();
    
    iThreadHost = new (ELeave) CHelperThreadHost(iBuffer, &(iBufferLock), &(iCommandQueue), &(iNotificationQueue));
    iThread.Create(KHelperThreadName, CHelperThreadHost::ThreadFunction, KDefaultStackSize, NULL, iThreadHost);
    iThread.Resume(); //new thread starts at ThreadFunction
    
    //wait until init is done
    TInt message = ENullMessage;
    iNotificationQueue.ReceiveBlocking(message);
    if(message != EInited){
        RDebug::Print(_L("expecting %d, got %d"), EInited, message);
    }
}

CAudioDevice* CAudioDevice::NewL(){
    CAudioDevice* self = new (ELeave) CAudioDevice();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
}

/*
 * Request to record samples.
 */
void CAudioDevice::ResumeRecording(){
    iCommandQueue.SendBlocking(EStartRecording);
    
    TInt message = ENullMessage;
    iNotificationQueue.ReceiveBlocking(message);
    if(message != ERecordingStarted){
        RDebug::Print(_L("expecting %d, got %d"), ERecordingStarted, message);
    }
}

/*
 * Request to stop recording samples. Note that actually we don't stop the recording,
 * but just discard incoming data until ResumeRecording is called again.
 */
void CAudioDevice::PauseRecording(){
    iCommandQueue.SendBlocking(EStopRecording);
    
    TInt message = ENullMessage;
    iNotificationQueue.ReceiveBlocking(message);
    if(message != ERecordingStopped){
        RDebug::Print(_L("expecting %d, got %d"), ERecordingStopped, message);
    }
}

/*
 * Reads at most maxSamples samples into destinationBuffer, returning
 * the actual number of samples read.
 */
TInt CAudioDevice::ReadSamples(TAny* aDestinationBuffer, TInt aMaxSamples){
    iBufferLock.Wait();
        TInt availableSamples = iBuffer->Size() / KBytesPerSample;
        TInt samplesToCopy = aMaxSamples;
        if (availableSamples < aMaxSamples){
            samplesToCopy = availableSamples;
        }
        TInt bytesToCopy = samplesToCopy * KBytesPerSample;
        iBuffer->Read(0, aDestinationBuffer, bytesToCopy);
        iBuffer->Delete(0, bytesToCopy);
    iBufferLock.Signal();

    return samplesToCopy;
}

CAudioDevice::~CAudioDevice(){
    //tell the thread to stop operations
    iCommandQueue.SendBlocking(EClose);

    TInt message = ENullMessage;
    iNotificationQueue.ReceiveBlocking(message);
    if(message != EClosed){
        RDebug::Print(_L("expecting %d, got %d"), EClosed, message);
    }
    
    //join thread
    TRequestStatus status;
    iThread.Logon(status);
    User::WaitForRequest(status);
    
    //destroy fields
    delete iThreadHost;
    iThread.Close();
    iBufferLock.Close();
    delete iBuffer;
    iNotificationQueue.Close();
    iCommandQueue.Close();
}

CHelperThreadHost::CHelperThreadHost(CBufSeg* aBuffer, RFastLock* aBufferLock, RMsgQueue<TInt>* aCommandQueue, RMsgQueue<TInt>* aNotificationQueue){
    iBuffer = aBuffer;
    iBufferLock = aBufferLock;
    iCommandQueue = aCommandQueue;
    iNotificationQueue = aNotificationQueue;
    iState = EPaused;
}

TInt CHelperThreadHost::ThreadFunction(TAny* aParam){
    CHelperThreadHost* host = (CHelperThreadHost*) aParam;

    //add cleanup stack support
    CTrapCleanup* cleanupStack = CTrapCleanup::New();
    
    //add active objects suppport
    TRAPD(error,   
        CActiveScheduler* activeScheduler = new (ELeave) CActiveScheduler;
        CleanupStack::PushL(activeScheduler);
        CActiveScheduler::Install(activeScheduler);
        
        //init multimedia system
        host->InitializeL();
        
        //run active scheduler
        CActiveScheduler::Start();
        
        //thread execution ended
        CleanupStack::PopAndDestroy(activeScheduler);
    );
    if(error != KErrNone){
        RDebug::Print(_L("thread error: %d"), error);
    }
    
    delete cleanupStack;
    return KErrNone;
}

/*
 * Inits iStream and iTemporaryBuffer.
 */
void CHelperThreadHost::InitializeL(){
    iStream = CMdaAudioInputStream::NewL(*this, EMdaPriorityMax, EMdaPriorityPreferenceTime);
    iStream->Open(&(iStreamSettings)); //calls MaiscOpenComplete asynchronously
    iTemporaryBuffer.CreateL(KTemporaryBufferSize);
}

/*
 * Destroys iStream and iTemporaryBuffer.
 */
void CHelperThreadHost::DestroyL(){
    iTemporaryBuffer.Close();
#if defined(__WINSCW__)
    iStream->Stop();
    CMdaAudioInputStream::Delete(iStream);
#else
    delete iStream;
#endif
}

/*
 * Called by the OS when iStream has been opened.
 */
void CHelperThreadHost::MaiscOpenComplete(TInt aError){
    if (aError == KErrNone){
        iNotificationQueue->SendBlocking(EInited);
                
        iStream->SetAudioPropertiesL(TMdaAudioDataSettings::ESampleRate16000Hz, TMdaAudioDataSettings::EChannelsMono);
        iStream->SetGain(iStream->MaxGain());

        iStream->ReadL(iTemporaryBuffer); //calls MaiscBufferCopied asynchronously
    }
    else{
        RDebug::Print(_L("error %d in MaiscOpenComplete"), aError);
    }
}

/*
 * Called by the OS when iTemporaryBuffer has been filled.
 */
void CHelperThreadHost::MaiscBufferCopied(TInt aError, const TDesC8 &aBuffer){
    if (aError == KErrNone){
        //if needed, record data
        if(iState == ERecording){
            TInt availableBytes = aBuffer.Size();
            iBufferLock->Wait();
                TInt bufferSize = iBuffer->Size();
                iBuffer->ExpandL(bufferSize, availableBytes);
                iBuffer->Write(bufferSize, aBuffer, availableBytes);
            iBufferLock->Signal();
        }
        
        //empty buffer
        iTemporaryBuffer.Zero();
        
        //process pending messages
        TInt message = ENullMessage;
        TInt result = iCommandQueue->Receive(message);
        if (result == KErrNone){
            if(message == EStartRecording){
                iState = ERecording;
                iNotificationQueue->SendBlocking(ERecordingStarted);
            }
            else if(message == EStopRecording){
                iState = EPaused;
                iNotificationQueue->SendBlocking(ERecordingStopped);
            }
            else if(message == EClose){
                iState = EClosing;
                iStream->Stop(); //calls MaiscRecordComplete asynchronously
                this->DestroyL();
                iNotificationQueue->SendBlocking(EClosed);
                User::Exit(0);
            }
            else{
                RDebug::Print(_L("received unexpected %d"), message);
            }
        }
        
        //unless stopping, request filling the next buffer
        if (iState != EClosing){
            iStream->ReadL(iTemporaryBuffer); //calls MaiscBufferCopied asynchronously
        }
    }
    else if (aError == KErrAbort){
        //sent when discarding data during close, nothing to do here
    }
    else{
        RDebug::Print(_L("error %d in MaiscBufferCopied"), aError);
    }
}

/*
 * Should be called by the OS when the recording is finished.
 * Due to a bug, this method never gets called.
 * http://carbidehelp.nokia.com/help/index.jsp?topic=/S60_5th_Edition_Cpp_Developers_Library/GUID-441D327D-D737-42A2-BCEA-FE89FBCA2F35/AudioStreamExample/doc/index.html
 */
void CHelperThreadHost::MaiscRecordComplete(TInt aError){
    //nothing to do here
}

CHelperThreadHost::~CHelperThreadHost(){
    //nothing to do here
}

//Sphinxbase methods

ad_rec_t* ad_open(void){
    ad_rec_t* result = new ad_rec_t;
    result->recorder = CAudioDevice::NewL();
    result->recording = FALSE;
    result->sps = KSampleRate;
    result->bps = KBytesPerSample;
    return result;
}

ad_rec_t* ad_open_dev(const char* dev, int32 sps){
    //dummy
    return ad_open();
}

ad_rec_t* ad_open_sps(int32 sps){
    //dummy
    return ad_open();
}

ad_rec_t* ad_open_sps_bufsize(int32 sps, int32 bufsize_msec){
    //dummy
    return ad_open();
}

int32 ad_start_rec(ad_rec_t* r){
    ((CAudioDevice*)r->recorder)->ResumeRecording();
    r->recording = TRUE;
    return AD_OK;
}

int32 ad_read(ad_rec_t* r, int16* buf, int32 max){
    int32 result = (int32) ((CAudioDevice*)r->recorder)->ReadSamples((TAny*) buf, (TInt)max);
    if(result == 0 && r->recording == FALSE){
        result = AD_EOF;
    }
    return result;
}

int32 ad_stop_rec(ad_rec_t* r){
    ((CAudioDevice*)r->recorder)->PauseRecording();
    r->recording = FALSE;
    return AD_OK;
}

int32 ad_close(ad_rec_t* r){
    delete ((CAudioDevice*)r->recorder);
    delete r;
    return AD_OK;
}

#endif //defined(AD_BACKEND_S60)
