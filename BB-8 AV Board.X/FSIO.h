/******************************************************************************
 *
 *                Microchip Memory Disk Drive File System
 *
 ******************************************************************************
 * FileName:        FSIO.h
 * Dependencies:    GenericTypeDefs.h
 *					FSconfig.h
 *					FSDefs.h
 *					stddef.h
 * Processor:       PIC18/PIC24/dsPIC30/dsPIC33
 * Compiler:        C18/C30
 * Company:         Microchip Technology, Inc.
 * Version:         1.0.0
 *
 * Software License Agreement
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the “Company”) for its PICmicro® Microcontroller is intended and
 * supplied to you, the Company’s customer, for use solely and
 * exclusively on Microchip PICmicro Microcontroller products. The
 * software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
*****************************************************************************/

#ifndef  FS_DOT_H
#define  FS_DOT_H

#include "GenericTypeDefs.h"
#include "FSconfig.h"
#include "FSDefs.h"
#include "stddef.h"

#ifdef USE_PIC18
	#include <p18cxxx.h>
#elif defined USE_PIC24
	#ifdef USE_PIC24F
		#include <p24fxxxx.h>
	#elif defined USE_PIC24H
		#include <p24hxxxx.h>
	#elif defined USE_PIC30
		#include <p30fxxxx.h>
	#elif defined USE_PIC33
		#include <p33fxxxx.h>
	#endif
#endif



/*******************************************************************/
/*                     Strunctures and defines                     */
/*******************************************************************/


//#ifdef USE_PIC18
//	#define NULL 0
//#endif

#define FALSE	0
#define TRUE	!FALSE

#ifndef SEEK_SET
	#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
	#define SEEK_CUR 1
#endif
#ifndef SEEK_END
	#define SEEK_END 2
#endif

#define APPEND "a"
#define WRITE "w"
#define READ "r"

#ifndef intmax_t
	#ifdef __PIC24F__
		#define intmax_t long long
	#elif defined __PIC24H__
		#define intmax_t long long
	#elif defined __dsPIC30F__
		#define intmax_t long long
	#elif defined __dsPIC33F__
		#define intmax_t long long
	#endif
#endif

typedef struct
{
    unsigned    write :1;           // set if the file was opened in a write mode 
	unsigned    FileWriteEOF :1;    // set if we are writing and have reached the EOF
}FILEFLAGS;

#define FILE_NAME_SIZE	    11

typedef struct 
{
    DISK *      	dsk;            // disk structure
    WORD         	cluster;        // first cluster
    WORD         	ccls;           // current cluster in file
    WORD         	sec;            // sector in current cluster
    WORD         	pos;            // position in current sector
    DWORD         	seek;           // position in the file
    DWORD         	size;           // file size
    FILEFLAGS   	flags;			// Write and EOF flags
    WORD     		time;           // last update time
    WORD     		date;           // last update date
    char     		name[FILE_NAME_SIZE];       // Needed to make it WORD wide for external mem
    WORD     		entry;          // entry position in cur directory
    WORD     		chk;            // FILE structure checksum = ~( entry + name[0])
    WORD     		attributes;     // the bare bones attributes
    WORD     		dirclus;        // base cluster of directory
    WORD     		dirccls;        // current cluster
} FSFILE;

typedef struct
{
	// User accessed values
	char			filename[FILE_NAME_SIZE + 2];	// File name
	unsigned char	attributes;		// The file's attributes
	unsigned long	filesize;		// Size of the file
	unsigned long 	timestamp;		// File's create time
	// For internal use only
	unsigned int 	entry;			// The file entry
	char			searchname[FILE_NAME_SIZE + 2];	// Search string
	unsigned char	searchattr;		// The search attributes
	unsigned int	cwdclus;		// The cwd for this search
	unsigned char	initialized;	// Check for if FindFirst was called
} SearchRec;


/***************************************************************************
* Prototypes                                                               *
***************************************************************************/

int FSInit(void);

FSFILE * FSfopen(const char * fileName, const char *mode);

#ifdef ALLOW_PGMFUNCTIONS
	FSFILE * FSfopenpgm(const rom char * fileName, const rom char *mode);
	int FSremovepgm (const rom char * fileName);
	int FindFirstpgm (const rom char * fileName, unsigned int attr, SearchRec * rec);
	int FSmkdirpgm (const rom char * path);
	int FSchdirpgm (const rom char * path);
	int FSrmdirpgm (const rom char * path, unsigned char rmsubdirs);
    int FSrenamepgm (const rom char * fileName, FSFILE * fo);
#endif


int FSfclose(FSFILE *fo);
//returns 0 on success. It returns EOF if any errors were detected.

int FSremove (const char * fileName);
//returns 0 on success, otherwise returns -1

void FSrewind (FSFILE *fo);

size_t FSfread(void *ptr, size_t size, size_t n, FSFILE *stream);
//On success fread returns the number of items (not bytes) actually read.
//On end-of-file or error it returns a short count or 0

size_t FSfwrite(const void *ptr, size_t size, size_t n, FSFILE *stream);
//On successful completion fwrite returns the number of items (not bytes) actually written.
//On error it returns a short count or 0.

int FSfseek(FSFILE *stream, long offset, int whence);
// return 0 if success. returns -1 on error

long FSftell(FSFILE *fo);
// returns the current file pointer position on success.
// It returns -1L on error.

int FSfeof( FSFILE * stream );
// Returns non-zero if EOF reached, else returns 0

#ifdef ALLOW_FORMATS
int FSformat (char mode, long int serialNumber, char * volumeID);
// Mode: 1- write new boot sec and erase FAT and root, 0 - just erase FAT and root
// returns 0 on success, otherwise returns EOF
#endif

int FSrename (const char * fileName, FSFILE * fo);
// Rename a file to fileName
// Returns 0 on success, -1 otherwise
// If dirs are enabled, passing in a NULL pointer for the second argument will 
//    rename the current working directory

#ifdef ALLOW_DIRS

int FSchdir (char * path);
// Returns 0 if successful, otherwise returns EOF

char * FSgetcwd (char * path, int numbchars);
// Returns NULL if unsuccessful, otherwise returns pointer to
// string containing cwd path name

int FSmkdir (char * path);
// Returns 0 if successful, otherwise returns EOF

int FSrmdir (char * path, unsigned char rmsubdirs);
// Returns 0 if successful, otherwise returns EOF

#endif

#ifdef USERDEFINEDCLOCK
int SetClockVars (unsigned int year, unsigned char month, unsigned char day, unsigned char hour, unsigned char minute, unsigned char second);
// Set the clock variables
// Returns 0 if values are valid, -1 otherwise
// Year 1980-2107, month 1-12, day 1-31, hour 0-23, minute 0-59, second 0-59
#endif

#ifdef ALLOW_FILESEARCH
int FindFirst (const char * fileName, unsigned int attr, SearchRec * rec); 
// Returns 0 if successful, -1 otherwise
// Your SearchRec structure must be initialized with FindFirst before
// calling FindNext or FindPrev
// fileName can be in the format *.*, *.EXT, FILENAME.*, or FILENAME.EXT
// If you are searching for a dir, filename can be of the format
//		FILENAME or *

int FindNext (SearchRec * rec); 
#endif

#ifdef ALLOW_FSFPRINTF
	#ifdef USE_PIC18
		int FSfprintf (FSFILE *fptr, const rom char *fmt, ...);
	#else
		int FSfprintf (FSFILE *fptr, const char * fmt, ...);
	#endif
#endif



#endif
