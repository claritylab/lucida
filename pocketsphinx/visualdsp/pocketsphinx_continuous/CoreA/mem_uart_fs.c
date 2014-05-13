///////////////////////////////////////////////////////////////
// 
// STDIO_FS
//
// STDIO Device Driver using host\FILESERVER over TCP/IP to
// implement the file actions (fopen(), fread(), etc).
//
///////////////////////////////////////////////////////////////

#include <device.h>
#include <ccblkfn.h>

#ifdef __ADSPBF533__
#include <cdefBF533.h>
#endif

#ifdef __ADSPBF561__
#include <cdefBF561.h>
#endif

#include "uart.h"

#include <stdio.h>
#include <_stdio.h>
#include <string.h>
#include <stdlib.h>
#include <device_int.h>
#include "model.h"
#include "err.h"

// #define DUMPTOUART
#define DUMPTOMEM

#if defined(DUMPTOMEM) && defined(DUMPTOUART)
#error Only either DUMPTOMEM or DUMPTOUART can be defined!
#endif

#ifdef DUMPTOMEM
#define MEMDUMPSIZE (1024*1024)
char dump_buffer[MEMDUMPSIZE];
#endif



#define min(a,b) ((a)>(b)?(b):(a))

static int   FS_init  (struct DevEntry *dev);
static int   FS_open  (const char *name, int mode);
static int   FS_close (int fd);
static int   FS_write (int fd, unsigned char *buf, int size);
static int   FS_read  (int fd, unsigned char *buf, int size);
static long   FS_seek  (int fd, long offset, int whence);

static DevEntry FS_DevEntry =
{
	0,
	NULL,
	FS_init,
	FS_open,
	FS_close,
	FS_write,
	FS_read,
	FS_seek,
	dev_not_claimed,
	dev_not_claimed,
	dev_not_claimed
};

struct fileentry
{
    const struct memoryfile* fileptr;
    int memory;
    int pos;
    int used;
};

#define MAXOPENFILES 8 // max open files
struct fileentry memfiletbl[MAXOPENFILES];

static char stdout_buf[256];
static char stderr_buf[256];

//
// accept one of stdin, stdout and stderr and reopen it as "con:" which is
// a special filename known to the file server
//
static int redirect(FILE* std)
{
    FILE* fp;
    
    fp = freopen("con:", "a+", std);
    if (!fp) {
        fprintf(stderr, "ERROR: failed to redirect '%s'\n",
               (std == stdin)? "stdin" : (std == stdout)? "stdout" : "stderr");
        return -1;
    }
    return 0;
}

//
// initialisation function for this STDIO device driver
//
// accepts an application-unique device number which it uses to install
// an entry into the runtime's device table; it then makes this driver the
// default one and redirects the standard files
//
int FS_STDIO_init(int devID)
{
    int a;
    // insert this uart fileserver client into the runtime's 
    // device table
    FS_DevEntry.DeviceID = devID;
    FS_DevEntry.data = (void*)0;
    int dev = add_devtab_entry(&FS_DevEntry);
    if (dev != devID) {
        fprintf(stderr, "ERROR: Unable to register new STDIO device\n");
        return -1;
    }
    
    for (a=0;a<MAXOPENFILES;a++)
    {
        memfiletbl[a].fileptr = NULL;
        memfiletbl[a].used	  = 0;
    }
    
    // make it the default STDIO device
    set_default_io_device(devID);
    if (get_default_io_device() != devID)
    	return -5;

    // freopen special filename "con:" as stdout and stderr
    //if (redirect(stdin))  return -2;
    if (redirect(stdout)) return -3;
    if (redirect(stderr)) return -4;
    
    // ensure stdout and stderr are line-buffered
    setvbuf(stdout, stdout_buf, _IOLBF, sizeof(stdout_buf));
    setvbuf(stderr, stderr_buf, _IOLBF, sizeof(stderr_buf));

    return 0;
}    

//
// called when this driver is inserted into the runtime's device table
//
static int   FS_init  ( struct DevEntry *dev )
{
    // just report the redirection using current STDIO
    //printf("STDIO being redirected to serial port");
           
	return 0;
}

//
// called when app does an fopen()
//
// allocate an entry in sock_tab, obtain a socket connection to the
// file server running elsewhere on the network and send it an 'open'
// request; return the index of the sock_tab entry (which the runtime
// then uses as the 'fid' parameter to FS_Read, FS_Write, etc)
//
static int   FS_open  (const char *name, int mode)
{
    // if filename is "con:" map directly to connection number 0; make
    // the connection if not already present
	int a,b;
    unsigned char temp[128];
    
	// our serial console
    if (strcmp(name, "con:")==0)
    {
        return 0;
    }
    
    // zero is reserved
    for (a=1;a<MAXOPENFILES;a++)
    {
        if (!memfiletbl[a].used)
        	break;
    }
    
    // no open handles left
    if (a>=MAXOPENFILES)
    	return -1;
    
    if ((mode&0xfff)==0)
	    for (b=0;strlen(modelfiles[b].name)!=0;b++)
	    {
	        if (strcmp(modelfiles[b].name,name)==0)
	        {
	            memfiletbl[a].pos 		= 0;
	            memfiletbl[a].used 		= 1;
	            memfiletbl[a].fileptr 	= modelfiles+b;
	            memfiletbl[a].memory 	= 1;
	            return a;
	        }
	    }

#ifdef DUMPTOUART
    if ((mode&0xfff)!=0)
    {
		sprintf(temp,"C:O:%d:%s\n",a,name);
	    UARTwrite(temp,strlen(temp));        
	 	memfiletbl[a].pos 		= 0;
		memfiletbl[a].used 		= 1;
	    memfiletbl[a].fileptr 	= 0;
	    memfiletbl[a].memory 	= 0;
	    return a;
    }
#endif

#ifdef DUMPTOMEM
    if ((mode&0xfff)!=0)
    {
	 	memfiletbl[a].pos 		= 0;
		memfiletbl[a].used 		= 1;
	    memfiletbl[a].fileptr 	= 0;
	    memfiletbl[a].memory 	= 1;
		sprintf(temp,"FILESERVER: Dumping file %s to memory.\n",name);
	    UARTwrite(temp,strlen(temp));
	    return a;
    }
#endif
    
    return -1;
}

static int   FS_write (int fd, unsigned char *buf, int size)
{
    unsigned const char *HEXVALS="0123456789ABCDEF";
    unsigned char tmp[16];
    
    if (fd==0)
    {
#ifdef DUMPTOUART	
        UARTwrite("0:",2);
#endif
    	UARTwrite(buf,size);
    	return size;
    }
    
    if (!memfiletbl[fd].used)
    	return -1;
    	
#ifdef DUMPTOMEM
	if (memfiletbl[fd].pos<MEMDUMPSIZE)
		if (size+memfiletbl[fd].pos<=MEMDUMPSIZE)
		{
			memcpy(dump_buffer+memfiletbl[fd].pos,buf,size);
			memfiletbl[fd].pos += size;
		}
		else
		{
			memcpy(dump_buffer+memfiletbl[fd].pos,buf,MEMDUMPSIZE-memfiletbl[fd].pos);
			memfiletbl[fd].pos = MEMDUMPSIZE;
		}
#endif    	
    
#ifdef DUMPTOUART	
    sprintf(tmp,"%d:%d\n",fd,size);
	UARTwrite(tmp,strlen(tmp) );
	UARTwrite(buf, size );
#endif

	return size;

}

static int   FS_read  (int fd, unsigned char *buf, int size)
{
    int readbytes=0;
    
	if (fd < 0 && fd >= MAXOPENFILES )
		return -1;
	
	if (!memfiletbl[fd].used)
		return -1;
		
    if (memfiletbl[fd].pos+size >
    	memfiletbl[fd].fileptr->length)
    	readbytes = memfiletbl[fd].fileptr->length-memfiletbl[fd].pos;
    else
    	readbytes = size;
    	
    if (readbytes == 0)
    	return -1; // EOF
    	
    memcpy(buf,memfiletbl[fd].fileptr->data+memfiletbl[fd].pos,readbytes);
    memfiletbl[fd].pos += readbytes;
		
    return readbytes;
}

static long   FS_seek  (int fd, long offset, int whence)
{
    if (fd==0)
    	return -1;
	if (!memfiletbl[fd].used)
		return -1;
	
	switch (whence)
	{
	    case SEEK_SET: memfiletbl[fd].pos = offset;break;
	    case SEEK_CUR: memfiletbl[fd].pos += offset;break;
	    case SEEK_END: memfiletbl[fd].pos = memfiletbl[fd].fileptr->length + offset;break;
	   	default: return -1;
	}
    
	return memfiletbl[fd].pos;
}

static int   FS_close (int fd)
{
    unsigned char temp[128];
   // ensure fd indexes a valid socket table entry
    if (fd < 0 || fd >= MAXOPENFILES)
        return -1;

#ifdef DUMPTOMEM
	sprintf(temp,"FILESERVER: Dump was done. Memloc is %x and length is %d\n",dump_buffer,memfiletbl[fd].pos);
    UARTwrite(temp,strlen(temp));
#endif

#ifdef DUMPTOUART
    if (!memfiletbl[fd].memory && memfiletbl[fd].used)
    {
		sprintf(temp,"C:C:%d\n",fd);
        UARTwrite(temp,strlen(temp));            
    }
#endif
    
    memfiletbl[fd].used = 0;
    return 0;
}


