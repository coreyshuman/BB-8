/******************************************************************************
*
*               Microchip Memory Disk Drive File System
*
******************************************************************************
* FileName:           FSIO.c
* Dependencies:       GenericTypeDefs.h
*               FSIO.h
*               Physical interface include file (SD.h, CF.h, ...)
*               string.h
*               stdlib.h
*               FSDefs.h
*                   ctype.h
*                   salloc.h
* Processor:          PIC18/PIC24/dsPIC30/dsPIC33
* Compiler:           C18/C30
* Company:            Microchip Technology, Inc.
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

#include "FSIO.h"
#include INCLUDEFILE
#include "GenericTypeDefs.h"
#include "string.h"
#include "stdlib.h"
#include "ctype.h"
#include "FSDefs.h"

#ifdef ALLOW_FSFPRINTF
    #include "stdarg.h"
#endif

#ifdef FS_DYNAMIC_MEM
    #ifdef USE_PIC18
        #include "salloc.h"
    #endif
#endif

#ifndef ALLOW_WRITES
    #ifdef ALLOW_DIRS
        #error Write functions must be enabled to use directory functions
    #endif
    #ifdef ALLOW_FORMATS
        #error Write functions must be enabled to use the format function
    #endif   
    #ifdef ALLOW_FSFPRINTF
        #error Write functions must be enabled to use the FSfprintf function
    #endif
#endif



/*****************************************************************************/
/*                         Global Variables                                  */
/*****************************************************************************/

// Define arrays for file allocation
#ifndef FS_DYNAMIC_MEM
    FSFILE  gFileArray[FS_MAX_FILES_OPEN];
    BYTE    gFileSlotOpen[FS_MAX_FILES_OPEN];
#endif

DWORD       gLastFATSectorRead = 0xFFFF;
BYTE        gNeedFATWrite = FALSE;
FSFILE  *   gBufferOwner = NULL;
DWORD       gLastDataSectorRead = 0xFFFFFFFF;
BYTE        gNeedDataWrite = FALSE;

// Timing variables
BYTE        gTimeCrtMS; // millisecond stamp
WORD        gTimeCrtTime;   // time created
WORD        gTimeCrtDate;   // date created
WORD        gTimeAccDate;   // Last Access date
WORD        gTimeWrtTime;   // last update time
WORD        gTimeWrtDate;   // last update date

// This boolean shows that the buffer contains all zeros
BYTE        gBufferZeroed = FALSE;

// Global current working 
#ifdef ALLOW_DIRS
    FSFILE   cwd;
    FSFILE * cwdptr = &cwd;
#endif

#ifdef USE_PIC18
    #pragma udata dataBuffer
    BYTE gDataBuffer[MEDIA_SECTOR_SIZE];
    #pragma udata FATBuffer
    BYTE gFATBuffer[MEDIA_SECTOR_SIZE];
#endif

#ifdef USE_PIC24
    BYTE __attribute__ ((aligned(4)))   gDataBuffer[MEDIA_SECTOR_SIZE];
    BYTE __attribute__ ((aligned(4)))   gFATBuffer[MEDIA_SECTOR_SIZE];
#endif


#pragma udata

DISK gDiskData;

/************************************************************************/
/*                        Structures and defines                        */
/************************************************************************/

#define PICKFILENAME 0
#define PICKDIRNAME 1

// Directory entry structure
typedef struct __DIRENTRY
{
    char      DIR_Name[DIR_NAMESIZE];      // Name
    char      DIR_Extension[DIR_EXTENSION]; // Extension
    BYTE      DIR_Attr;         // Attributes
    BYTE      DIR_NTRes;        // reserved by NT
    BYTE      DIR_CrtTimeTenth; // millisecond stamp
    WORD      DIR_CrtTime;      // time created
    WORD      DIR_CrtDate;      // date created
    WORD      DIR_LstAccDate;   // Last Access date
    WORD      DIR_FstClusHI;    // high word of this enty's first cluster number
    WORD      DIR_WrtTime;      // last update time
    WORD      DIR_WrtDate;      // last update date
    WORD      DIR_FstClusLO;    // low word of this entry's first cluster number
    DWORD     DIR_FileSize;     // file size in DWORD
}_DIRENTRY;

typedef _DIRENTRY * DIRENTRY;

// directory entry management
#define DIR_ESIZE   32      // size of a directory entry (BYTEs)
#define DIR_NAME   0       // offset of file name
#define DIR_EXT      8       // offset of file extension
#define DIR_ATTRIB   11      // offset of attribute( 00ARSHDV) (BYTE)
#define DIR_TIME   22      // offset of last use time  (WORD)
#define DIR_DATE   24      // offset of last use date  (WORD)
#define DIR_CLST   26      // offset of first cluster in FAT (WORD)
#define DIR_SIZE   28      // offset of file size (DWORD)
#define DIR_DEL      0xE5    // marker of a deleted entry
#define DIR_EMPTY   0       // marker of last entry in directory

#define DIRECTORY 0x12

// number of directory entries in one sector
#define DIRENTRIES_PER_SECTOR   0x10


// internal errors
#define CE_FAT_EOF            60   // fat attempt to read beyond EOF
#define CE_EOF               61   // reached the end of file   

// since we use an address generator, FILE is not actually the cast of what we pass
typedef FSFILE   * FILEOBJ;

#define SIZEOF(s,m)            ((size_t) sizeof(((s *)0)->m)) 


#ifdef ALLOW_FSFPRINTF

#define _FLAG_MINUS 0x1
#define _FLAG_PLUS  0x2
#define _FLAG_SPACE 0x4
#define _FLAG_OCTO  0x8
#define _FLAG_ZERO  0x10
#define _FLAG_SIGNED 0x80


#ifdef USE_PIC18
    #define _FMT_UNSPECIFIED 0
    #define _FMT_LONG 1
    #define _FMT_SHRTLONG 2
    #define _FMT_BYTE   3
#else
    #define _FMT_UNSPECIFIED 0
    #define _FMT_LONGLONG 1
    #define _FMT_LONG 2
    #define _FMT_BYTE 3
#endif

#ifdef USE_PIC18
    static const rom char s_digits[] = "0123456789abcdef";
#else
    static const char s_digits[] = "0123456789abcdef";
#endif

#endif



/************************************************************************************/
/*                               Prototypes                                         */
/************************************************************************************/

WORD ReadFAT (DISK *dsk, WORD ccls);
DIRENTRY Cache_File_Entry( FILEOBJ fo, WORD * curEntry, BYTE ForceRead);
BYTE Fill_File_Object(FILEOBJ fo, WORD *fHandle);
DWORD Cluster2Sector(DISK * disk, WORD cluster);
DIRENTRY LoadDirAttrib(FILEOBJ fo, WORD *fHandle);
#ifdef INCREMENTTIMESTAMP
    void IncrementTimeStamp(DIRENTRY dir);
#elif defined USEREALTIMECLOCK
    void CacheTime (void);
#endif

void FileObjectCopy(FILEOBJ foDest,FILEOBJ foSource);
BYTE isShort ( char aChar, BYTE mode);
BYTE isDIR (BYTE aChar, BYTE mode);
extern BYTE WriteProtectState(void);
BYTE ValidateChars (char * FileName, BYTE which, BYTE mode);
BYTE FormatFileName( const char* fileName, char* fN2, BYTE mode);
CETYPE FILEfind( FILEOBJ foDest, FILEOBJ foCompareTo, BYTE cmd, BYTE mode);
BYTE FILEget_next_cluster(FILEOBJ fo, WORD n);
CETYPE FILEopen (FILEOBJ fo, WORD *fHandle, char type);

// Write functions
#ifdef ALLOW_WRITES
    BYTE flushData (void);
    CETYPE FILEerase( FILEOBJ fo, WORD *fHandle, BYTE EraseClusters);
    BYTE FILEallocate_new_cluster( FILEOBJ fo, BYTE mode);
    BYTE FAT_erase_cluster_chain (WORD cluster, DISK * dsk);
    WORD FATfindEmptyCluster(FILEOBJ fo);
    BYTE FindEmptyEntries(FILEOBJ fo, WORD *fHandle);
    BYTE PopulateEntries(FILEOBJ fo, char *name , WORD *fHandle, BYTE mode);
    CETYPE FILECreateHeadCluster( FILEOBJ fo, WORD *cluster);
    BYTE EraseCluster(DISK *disk, WORD cluster);
    CETYPE CreateFirstCluster(FILEOBJ fo);
    WORD WriteFAT (DISK *dsk, WORD ccls, WORD value, BYTE forceWrite);
    CETYPE CreateFileEntry(FILEOBJ fo, WORD *fHandle, BYTE mode);
#endif

// Dir functions
#ifdef ALLOW_DIRS
    BYTE GetPreviousEntry (FILEOBJ fo);
    void FormatDirName (char * string, BYTE mode);
    int CreateDIR (char * path);
    BYTE writeDotEntries (DISK * dsk, WORD dotAddress, WORD dotdotAddress);
    int eraseDir (char * path);
    #ifdef ALLOW_PGMFUNCTIONS
        int mkdirhelper (BYTE mode, char * ramptr, const rom char * romptr);
        int chdirhelper (BYTE mode, char * ramptr, const rom char * romptr);
        int rmdirhelper (BYTE mode, char * ramptr, const rom char * romptr, unsigned char rmsubdirs);
    #else
        int mkdirhelper (char * path, char * ramptr, char * romptr);
        int chdirhelper (BYTE mode, char * ramptr, char * romptr);
        int rmdirhelper (BYTE mode, char * ramptr, char * romptr, unsigned char rmsubdirs);
    #endif
#endif

#ifdef ALLOW_FSFPRINTF
    #ifdef USE_PIC18
        int FSvfprintf (auto FSFILE *handle, auto const rom char *f, auto va_list ap);
    #else
        int FSvfprintf (FSFILE *handle, const char *f, va_list ap);
    #endif
    int FSputc (char c, FSFILE * f);
    unsigned char str_put_n_chars (FSFILE * handle, unsigned char n, char c);
#endif

BYTE DISKmount( DISK *dsk);
BYTE LoadMBR(DISK *dsk);
BYTE LoadBootSector(DISK *dsk);

extern void Delayms(BYTE milliseconds);


/******************************************************************************
* Function:        int FSInit()
*
* PreCondition:    None
*
* Input:           None
*
* Output:          int:   TRUE if initialization successful, otherwise FALSE
*
* Side Effects:    None
*
* Overview:        Initialize the static memory slots for holding
*               file structures; only used when FS_DYNAMIC_MEM is
*               not defined
*
* Note:            None
*****************************************************************************/
int FSInit()
{
    int fIndex;
#ifndef FS_DYNAMIC_MEM

    for( fIndex = 0; fIndex < FS_MAX_FILES_OPEN; fIndex++ )
    gFileSlotOpen[fIndex] = TRUE;

#else
    #ifdef USE_PIC18
        SRAMInitHeap();
    #endif
#endif

    gBufferZeroed = FALSE;

    InitIO();

    if(DISKmount(&gDiskData) == CE_GOOD)
    {
        // Initialize the current working directory to the root
        #ifdef ALLOW_DIRS
            cwdptr->dsk = &gDiskData;
            cwdptr->sec = 0;
            cwdptr->pos = 0;
            cwdptr->seek = 0;
            cwdptr->size = 0;
            cwdptr->name[0] = '\\';
            for (fIndex = 1; fIndex < 11; fIndex++)
            {
                cwdptr->name[fIndex] = 0x20;
            } 
            cwdptr->entry = 0;
            cwdptr->attributes = ATTR_DIRECTORY;
            // Zero indicates the root
            cwdptr->dirclus = 0;
            cwdptr->dirccls = 0;
        #endif
        
        return TRUE;

    }
return FALSE;
}

/***********************************************************************************************
* Function:        CETYPE FILEfind( FILEOBJ foDest, FILEOBJ foCompareTo, BYTE cmd, BYTE mode)
*
* PreCondition:    DISKmount function has been executed, foCompareTo loaded with
*                  file name
*
* Input:        foDest       - FILEOBJ containing information of file found
*               foCompareTo  - FILEOBJ containing name of file to be found
*               cmd          - 1 - search for a matching entry
*                              2 - search for an empty entry
*            mode         - 0 - match a file exactly, default attr
*                              1 - match with user attributes
*                  
* Output:          CE_GOOD             - File found
*                  CE_FILE_NOT_FOUND   - File not found 
*
* Side Effects:    None
*
* Overview:        Find a file given a name as passed via foCompareTo, place found in foDest
*
* Note:            None
************************************************************************************************/

CETYPE FILEfind( FILEOBJ foDest, FILEOBJ foCompareTo, BYTE cmd, BYTE mode)
{
    WORD    attrib, compareAttrib;
    WORD    fHandle = foDest->entry;      // current entry counter
    BYTE    state,index;              // state of the current object
    CETYPE  statusB = CE_FILE_NOT_FOUND;           
    BYTE    character,test;

    // reset the cluster
    foDest->dirccls = foDest->dirclus;
    compareAttrib = 0xFFFF ^ foCompareTo->attributes;
    if (fHandle == 0)
    {
        if (Cache_File_Entry(foDest, &fHandle, TRUE) == NULL)
        {
            statusB = CE_BADCACHEREAD;
        }
    }
    else
    {
        if ((fHandle & 0xf) != 0)
        {
            if (Cache_File_Entry (foDest, &fHandle, TRUE) == NULL)
            {
                statusB = CE_BADCACHEREAD;
            }
        }
    }

    if (statusB != CE_BADCACHEREAD)
    {
        // Loop until you reach the end or find the file 
        while(1)
        {
            if(statusB!=CE_GOOD)
            {
                state = Fill_File_Object(foDest, &fHandle);
                if(state == NO_MORE)
                {
                    break;
                }
            }
            else
            {
                break;
            }

            if(state == FOUND)
            {
                /* We got something */
                // get the attributes
                attrib = foDest->attributes;

                attrib &= ATTR_MASK;
                switch (mode)
                {
                    case 0:
                        // see if we are a volume id or hidden, ignore
                        if((attrib != ATTR_VOLUME) && (attrib & ATTR_HIDDEN) != ATTR_HIDDEN)
                        {
                            statusB = CE_GOOD;
                            character = (BYTE)'m'; // random value

                            // search for one. if status = TRUE we found one
                            for(index = 0; index < DIR_NAMECOMP; index++)
                            {
                                // get the source character 
                                character = foDest->name[index];
                                // get the destination character
                                test = foCompareTo->name[index];
                                if(tolower(character) != tolower(test))
                                {
                                    statusB = CE_FILE_NOT_FOUND; // Nope its not a match
                                    break;
                                }           
                            }// for loop
                        } // not dir nor vol
                        break;

                    case 1:
                        // Check for attribute match
                        if (((attrib & compareAttrib) == 0) && (attrib != ATTR_LONG_NAME))
                        {
                            statusB = CE_GOOD;
                            character = (BYTE)'m'; // random value
                            if (foCompareTo->name[0] != '*')
                            {         
                                for (index = 0; index < DIR_NAMESIZE; index++)
                                {
                                    // Get the source character
                                    character = foDest->name[index];
                                    // Get the destination character
                                    test = foCompareTo->name[index];
                                    if (test == '*')
                                        break;
                                    if(tolower(character) != tolower(test))
                                    {
                                        statusB = CE_FILE_NOT_FOUND; // it's not a match
                                        break;
                                    }   
                                }
                            }
                            if ((foCompareTo->name[8] != '*') && (statusB == CE_GOOD))
                            {
                                for (index = 8; index < DIR_NAMECOMP; index++)
                                {
                                    // Get the source character
                                    character = foDest->name[index];
                                    // Get the destination character
                                    test = foCompareTo->name[index];
                                    if (test == '*')
                                        break;
                                    if(tolower(character) != tolower(test))
                                    {
                                        statusB = CE_FILE_NOT_FOUND; // it's not a match
                                        break;
                                    }
                                }                        
                            }
                        } // Attribute match

                        break;
                }
            } // not found
            else
            {
                /*** looking for an empty/re-usable entry ***/
                if ( cmd == 0) 
                    statusB = CE_GOOD;
            } // found or not    

            // increment it no matter what happened
            fHandle++;

        }// while 
    }
    return(statusB);
} // FILEFind


/******************************************************************************
* Function:        CETYPE FILEopen (FILEOBJ fo, WORD *fHandle, char type)
*
* PreCondition:    FILEfind returns true for read or append, fo contains file data
*
* Input:           fo           - File to be opened
*                  fHandle      - Location of file
*                  type         - WRITE - Create a new file or replace an existing file
*                                 READ - Read data from an existing file
*                                 APPEND - Append data to an existing file
*                  
* Output:          CE_GOOD             - FILEopen successful
*                  CE_NOT_INIT         - Card is not yet initialized because of some error
*                  CE_FILE_NOT_FOUND   - Could not find the file on the card
*                  CE_BAD_SECTOR_READ  - A bad read occured of a sector  
*               
*
* Side Effects:    None
*
* Overview:        Opens a file to perform operations on
*
* Note:            None
*****************************************************************************/

CETYPE FILEopen (FILEOBJ fo, WORD *fHandle, char type)
{
    DISK *  dsk;                //Disk structure
    BYTE    r;                  //Result of search for file
    DWORD   l;                  //lba of first sector of first cluster
    CETYPE  error = CE_GOOD;

    dsk = (DISK *)(fo->dsk);
    if (dsk->mount == FALSE)
        error = CE_NOT_INIT;
    else
    {
        // load the sector
        fo->dirccls = fo->dirclus;
        // Cache no matter what if it's the first entry
        if (*fHandle == 0)
        {
            if (Cache_File_Entry(fo, fHandle, TRUE) == NULL)
            {
                error = CE_BADCACHEREAD;
            }
        }
        else
        {
            // If it's not the first, only cache it if it's
            // not divisible by the number of entries per sector
            // If it is, Fill_File_Object will cache it
            if ((*fHandle & 0xf) != 0)
            {
                if (Cache_File_Entry (fo, fHandle, TRUE) == NULL)
                {
                    error = CE_BADCACHEREAD;
                }
            }
        }

        // Fill up the File Object with the information pointed to by fHandle
        r = Fill_File_Object(fo, fHandle);
        if (r != FOUND)
            error = CE_FILE_NOT_FOUND;
        else
        {
            fo->seek = 0;               // first byte in file
            fo->ccls = fo->cluster;     // first cluster
            fo->sec = 0;                // first sector in the cluster
            fo->pos = 0;                // first byte in sector/cluster

            if  ( r == NOT_FOUND)
            {
                error = CE_FILE_NOT_FOUND;
            }
            else
            {   
                // Determine the lba of the selected sector and load 
                l = Cluster2Sector(dsk,fo->ccls);
#ifdef ALLOW_WRITES
                if (gNeedDataWrite)
                    if (flushData())
                        return EOF;
#endif
                gBufferOwner = fo;
                if (gLastDataSectorRead != l)
                {
                    gBufferZeroed = FALSE;
                    if ( !SectorRead( l, dsk->buffer))
                        error = CE_BAD_SECTOR_READ;                         
                    gLastDataSectorRead = l;
                }
            } // -- found

            fo->flags.FileWriteEOF = FALSE;
            // Set flag for operation type
#ifdef ALLOW_WRITES
            if (type == 'w' || type == 'a')
            {
                fo->flags.write = 1;   //write or append
            }
            else
            {
#endif
                fo->flags.write = 0;   //read
#ifdef ALLOW_WRITES
            } // -- flags
#endif
        } // -- r = Found
    } // -- Mounted
    return (error);
} // -- FILEopen


/******************************************************************************
* Function:        BYTE FILEget_next_cluster(FILEOBJ fo, WORD n)
*
* PreCondition:    Disk mounted, fo contains valid file data
*
* Input:           fo      - File structure
*                  n       - number of links in the FAT cluster chain to jump
*                            through; n == 1 - next cluster in the chain
*                  
* Output:          CE_GOOD              - Operation successful
*                  CE_BAD_SECTOR_READ   - A bad read occured of a sector 
*                  CE_INVALID_CLUSTER   - Invalid cluster value > maxcls 
*                  CE_FAT_EOF           - Fat attempt to read beyond EOF
*               
*
* Side Effects:    None
*
* Overview:        Steps through a chain of clusters
*
* Note:            None
*****************************************************************************/

BYTE FILEget_next_cluster(FILEOBJ fo, WORD n)
{
    WORD        c, c2;
    BYTE        error = CE_GOOD;
    DISK *      disk;

    disk = fo->dsk;     

    // loop n times
    do 
    {
        // get the next cluster link from FAT
        c2 = fo->ccls;
        if ( (c = ReadFAT( disk, c2)) == CLUSTER_FAIL)
            error = CE_BAD_SECTOR_READ;
        else
        {    
            // check if cluster value is valid
            if ( c >= disk->maxcls)
            {
                error = CE_INVALID_CLUSTER;
            }

            // compare against max value of a cluster in FAT
            if (disk->type == FAT16)
                c2 = LAST_CLUSTER_FAT16;
            else if (disk->type == FAT12)
                c2 = LAST_CLUSTER_FAT12;     

            // return if eof
            if ( c >= c2)    // check against eof
            {
                error = CE_FAT_EOF;
            }
        }

        // update the FSFILE structure
        fo->ccls = c;

    } while (--n > 0 && error == CE_GOOD);// loop end

    return(error);
} // get next cluster

/******************************************************************************
 * Function:        BYTE DISKmount ( DISK *dsk)
 *
 * PreCondition:    Called from FSInit()
 *
 * Input:           dsk		- The disk to be mounted
 *
 * Output:          CE_GOOD 		- Disk mounted
 *					CE_INIT_ERROR	- Initialization error has occured
 *
 * Side Effects:    None
 *
 * Overview:        Will mount only the first partition on the disk/card
 *
 * Note:            None
 *****************************************************************************/

BYTE DISKmount( DISK *dsk)
{
    BYTE        error = CE_GOOD;
    
    dsk->mount = FALSE; // default invalid
    
    dsk->buffer = gDataBuffer;    // assign buffer
    
    // Initialize the device
    if(MediaInitialize() != TRUE)
    {
        error = CE_INIT_ERROR;
    }
    else
    {
        // Load the Master Boot Record (partition)
        if((error = LoadMBR(dsk)) == CE_GOOD)
        {
            // Now the boot sector
            if((error = LoadBootSector(dsk)) == CE_GOOD)
                dsk->mount = TRUE; // Mark that the DISK mounted successfully
        }
    
    } // -- Load file parameters
    
    return(error);
} // -- mount

/******************************************************************************
 * Function:        CETYPE LoadMBR ( DISK *dsk)
 *
 * PreCondition:    Called from DISKmount
 *
 * Input:           dsk		- The disk containing the master boot record to be loaded
 *
 * Output:          CE_GOOD 			- MBR loaded successfully
 *                  CE_BAD_SECTOR_READ	- A bad read occured of a sector 
 *					CE_BAD_PARTITION	- The boot record is bad
 *
 * Side Effects:    None
 *
 * Overview:        Loads the MBR and extracts the necessary information
 *
 * Note:            None
 *****************************************************************************/

BYTE LoadMBR(DISK *dsk)
{
    PT_MBR  Partition;
    BYTE error = CE_GOOD;
    BYTE type;
    BootSec BSec;
    
    // Get the partition table from the MBR
    if ( SectorRead( FO_MBR, dsk->buffer) != TRUE) 
        error = CE_BAD_SECTOR_READ;
    else
    {
        // Check if the card has no MBR
        BSec = (BootSec) dsk->buffer;
        
        if((BSec->Signature0 == FAT_GOOD_SIGN_0) && (BSec->Signature1 == FAT_GOOD_SIGN_1))
        {
            // Technically, the OEM name is not for indication
            // The alternative is to read the CIS from attribute
            // memory.  See the PCMCIA metaformat for more details
#ifdef USE_PIC24
            if (ReadByte( dsk->buffer, BSI_FSTYPE ) == 'F' && \
                ReadByte( dsk->buffer, BSI_FSTYPE + 1 ) == 'A' && \
                ReadByte( dsk->buffer, BSI_FSTYPE + 2 ) == 'T' && \
                ReadByte( dsk->buffer, BSI_FSTYPE + 3 ) == '1' && \
                ReadByte( dsk->buffer, BSI_BOOTSIG) == 0x29)
#else
            if (BSec->FAT.FAT_16.BootSec_FSType[0] == 'F' && \
                BSec->FAT.FAT_16.BootSec_FSType[1] == 'A' && \
                BSec->FAT.FAT_16.BootSec_FSType[2] == 'T' && \
                BSec->FAT.FAT_16.BootSec_FSType[3] == '1' && \
                BSec->FAT.FAT_16.BootSec_BootSig == 0x29)
#endif
            {
                dsk->firsts = 0;
                dsk->type = FAT16;
            }
        }
        // assign it the partition table strucutre
        Partition = (PT_MBR)dsk->buffer;
        
        // Ensure its good
        if((Partition->Signature0 != FAT_GOOD_SIGN_0) || (Partition->Signature1 != FAT_GOOD_SIGN_1))
        {
            error = CE_BAD_PARTITION;
        }
        else
        {    
            /*    Valid Master Boot Record Loaded   */
            
            // Get the 32 bit offset to the first partition 
            dsk->firsts = Partition->Partition0.PTE_FrstSect; 
            
            // check if the partition type is acceptable
            type = Partition->Partition0.PTE_FSDesc;
        
            switch (type)
            {
                case 0x01:
                    dsk->type = FAT12;
                    break;

                case 0x04:
                case 0x06:
                case 0x0E:
                    dsk->type = FAT16;
                    break;

                case 0x0B:
                case 0x0C:
                    dsk->type = FAT32; // and error out				
                    error = CE_CARDFAT32;
                    break;

                default:
                    error = CE_BAD_PARTITION;
            } // switch
        }
    }
    
    return(error);
}// -- LoadMBR


/******************************************************************************
 * Function:        BYTE LoadBootSector (DISK *dsk)
 *
 * PreCondition:    Called from DISKmount
 *
 * Input:           dsk		- The disk containing the boot sector
 *                  
 * Output:          CE_GOOD 			- Boot sector loaded
 *                  CE_BAD_SECTOR_READ	- A bad read occured of a sector 
 *					CE_NOT_FORMATTED	- The disk is of an unsupported format
 *					CE_CARDFAT12		- FAT12 during intial testing we are not supporting FAT12
 *					CE_CARDFAT32		- FAT 32 - card not supported
 *
 * Side Effects:    None
 *
 * Overview:        Load the boot sector information and extract the necessary information
 *
 * Note:            None
 *****************************************************************************/


BYTE LoadBootSector(DISK *dsk)
{
    WORD        RootDirSectors;
    DWORD       TotSec,DataSec;
    BYTE        error = CE_GOOD;
    BootSec     BSec;        // boot sector, assume its FAT16 til we know better
    WORD   BytesPerSec;

    // Get the Boot sector
    if ( SectorRead( dsk->firsts, dsk->buffer) != TRUE) 
        error = CE_BAD_SECTOR_READ;  
    else
    {
        // Assign the type
        BSec = (BootSec)dsk->buffer;
    
        //Verify the Boot Sector is valid
        if((BSec->Signature0 != FAT_GOOD_SIGN_0) || (BSec->Signature1 != FAT_GOOD_SIGN_1))
        {
            error = CE_NOT_FORMATTED;
        }
        else
        {     
            // determine the number of sectors in one FAT
            #ifdef USE_PIC18
                dsk->fatsize = BSec->FAT.FAT_16.BootSec_SPF;  
            #else
                dsk->fatsize = ReadWord( dsk->buffer, BSI_SPF ); 
            #endif
    
            // Figure out the total number of sectors
            #ifdef USE_PIC18
                if(BSec->FAT.FAT_16.BootSec_TotSec16 != 0)
                {
                    TotSec = BSec->FAT.FAT_16.BootSec_TotSec16;
                }
                else
                {
                    TotSec = BSec->FAT.FAT_16.BootSec_TotSec32;
                }
            #else
                TotSec = ReadWord( dsk->buffer, BSI_TOTSEC16 );
                if( TotSec == 0 )
                    TotSec = ReadDWord( dsk->buffer, BSI_TOTSEC32 );
            #endif
    
    
            #ifdef USE_PIC18
                // get the full partition/drive layout
                // determine the size of a cluster
                dsk->SecPerClus = BSec->FAT.FAT_16.BootSec_SPC;
    
                // determine fat, root and data lbas
                // FAT = first sector in partition (boot record) + reserved records
                dsk->fat        = dsk->firsts + BSec->FAT.FAT_16.BootSec_ResrvSec;
                
                // fatcopy is the number of FAT tables 
                dsk->fatcopy    = BSec->FAT.FAT_16.BootSec_FATCount;
                
                // maxroot is the maximum number of entries in the root directory
                dsk->maxroot    = BSec->FAT.FAT_16.BootSec_RootDirEnts;    
                
                // root Is the sector location of the root directory
                dsk->root = dsk->fat + (dsk->fatcopy * dsk->fatsize);
                
                BytesPerSec = BSec->FAT.FAT_16.BootSec_BPS;
                
                if( BytesPerSec == 0 || (BytesPerSec & 1) == 1 ) //cannot be 0 or odd
                    return( CE_NOT_FORMATTED );
                
                RootDirSectors = ((BSec->FAT.FAT_16.BootSec_RootDirEnts * 32) + (BSec->FAT.FAT_16.BootSec_BPS - 1)) / BSec->FAT.FAT_16.BootSec_BPS;                    
    
            #else
    
                dsk->SecPerClus = ReadByte( dsk->buffer, BSI_SPC );
                
                // determine fat, root and data lbas
                // FAT = first sector in partition (boot record) + reserved records
                dsk->fat        = dsk->firsts + ReadWord( dsk->buffer, BSI_RESRVSEC );
                
                // fatcopy is the number of FAT tables 
                dsk->fatcopy    = ReadByte( dsk->buffer, BSI_FATCOUNT );
                
                // maxroot is the maximum number of entries in the root directory
                dsk->maxroot    = ReadWord( dsk->buffer, BSI_ROOTDIRENTS );   
                
                // root Is the sector location of the root directory
                dsk->root = dsk->fat + (dsk->fatcopy * dsk->fatsize);
                
                BytesPerSec = ReadWord( dsk->buffer, BSI_BPS );
                
                if( BytesPerSec == 0 || (BytesPerSec & 1) == 1 ) //cannot be 0 or odd
                    return( CE_NOT_FORMATTED );
                
                RootDirSectors = ((dsk->maxroot * 32) + (BytesPerSec - 1)) / BytesPerSec;               
            #endif
    
            // figure out how many data sectors there are
            DataSec = TotSec - (dsk->root + RootDirSectors) + dsk->firsts + 2;
            
            dsk->maxcls = DataSec / dsk->SecPerClus;
            
            // Calculate FAT type
            if(dsk->maxcls < 4085)
            {
                /* Volume is FAT12 */
                dsk->type = FAT12;
            }
            else 
            {
                if(dsk->maxcls < 65525)
                {
                    /* Volume is FAT16 */
                    dsk->type = FAT16;
                }
                else
                    /* Volume is FAT32 */
                    // We don't support FAT32
                    error = CE_CARDFAT32;     
            }
    
            // DATA = ROOT + (MAXIMUM ROOT *32 / 512) 
            dsk->data = dsk->root + ( dsk->maxroot >> 4); // assuming maxroot % 16 == 0
    
            // make sure that we can read in a complete sector
            #ifdef USE_PIC18
                if(BSec->FAT.FAT_16.BootSec_BPS != MEDIA_SECTOR_SIZE)
            #else
                if(BytesPerSec != MEDIA_SECTOR_SIZE)
            #endif
            error = CE_NOT_FORMATTED;
        }
    }
    
    return(error);
}

/******************************************************************************
 * Function:        int FSformat (char mode, long int serialNumber, char * volumeID)
 *
 * PreCondition:    InitIO complete for the required physical media
 *
 * Input:           mode	- 1: Create a new boot sector
 *							  0: Just erase FAT and root
 *					serialNumber	- Serial number to write to the card
 *					volumeID		- Name of the card
 *
 * Output:			int		- Returns 0 on success, EOF on failure
 *
 * Side Effects:    None
 *
 * Overview:        Reads a byte from a buffer
 *
 * Note:            None
 *****************************************************************************/

#ifdef ALLOW_FORMATS
#ifdef ALLOW_WRITES
int FSformat (char mode, long int serialNumber, char * volumeID)
{
	PT_MBR	masterBootRecord;
	DWORD 	secCount, FAT16DataClusters, RootDirSectors;
	BootSec	BSec;
	DISK	d;
	DISK * disk = &d;
	WORD 	i, j;
	DWORD	fatsize, test;
#ifdef USE_PIC18
	// This is here because of a C18 compiler feature
	BYTE *  dataBufferPointer = gDataBuffer;
#endif

	disk->buffer = gDataBuffer;

    InitIO();

	if (MediaInitialize() != TRUE)
		return EOF;

	if (SectorRead (0x00, gDataBuffer) == FALSE)
		return EOF;

	// Check if the card has no MBR
	BSec = (BootSec) disk->buffer;
	if((BSec->Signature0 == FAT_GOOD_SIGN_0) && (BSec->Signature1 == FAT_GOOD_SIGN_1))
	{
		// Technically, the OEM name is not for indication
		// The alternative is to read the CIS from attribute
		// memory.  See the PCMCIA metaformat for more details
#ifdef USE_PIC24
		if (ReadByte( disk->buffer, BSI_FSTYPE ) == 'F' && \
			ReadByte( disk->buffer, BSI_FSTYPE + 1 ) == 'A' && \
			ReadByte( disk->buffer, BSI_FSTYPE + 2 ) == 'T' && \
			ReadByte( disk->buffer, BSI_FSTYPE + 3 ) == '1' && \
			ReadByte( disk->buffer, BSI_BOOTSIG) == 0x29)
#else
		if (BSec->FAT.FAT_16.BootSec_FSType[0] == 'F' && \
			BSec->FAT.FAT_16.BootSec_FSType[1] == 'A' && \
			BSec->FAT.FAT_16.BootSec_FSType[2] == 'T' && \
			BSec->FAT.FAT_16.BootSec_FSType[3] == '1' && \
			BSec->FAT.FAT_16.BootSec_BootSig == 0x29)
#endif
		{
			switch (mode)
			{
				case 1:
					// not enough info to construct our own boot sector
					return EOF;
					break;
				case 0:
				// We have to determine the operating system, and the 
				// locations and sizes of the root dir and FAT, and the 
				// count of FATs
					disk->firsts = 0;
					if (LoadBootSector (disk) != CE_GOOD)
						return EOF;

				default:
					break;
			}
		}
		else
		{	
			masterBootRecord = (PT_MBR) &gDataBuffer;
			disk->firsts = masterBootRecord->Partition0.PTE_FrstSect;
		}
		
	}


	switch (mode)
	{
		// True: Rewrite the whole boot sector
		case 1:
			secCount = masterBootRecord->Partition0.PTE_NumSect;
			
			if (secCount < 0x1039)
			{
				disk->type = FAT12;
				// Format to FAT12 only if there are too few sectors to format
				// as FAT16
				masterBootRecord->Partition0.PTE_FSDesc = 0x01;
				if (SectorWrite (0x00, gDataBuffer, TRUE) == FALSE)
					return EOF;
				
				if (secCount >= 0x1028)
				{
					// More than 0x18 sectors for FATs, 0x20 for root dir, 
					// 0x8 reserved, and 0xFED for data
					// So double the number of sectors in a cluster to reduce
					// the number of data clusters used
					disk->SecPerClus = 2;
				}
				else
				{
					// One sector per cluster
					disk->SecPerClus = 1;
				}
				
				// Prepare a boot sector
				memset (gDataBuffer, 0x00, MEDIA_SECTOR_SIZE);

				// Last digit of file system name (FAT12   )
				gDataBuffer[58] = '2';

			}
			else if (secCount <= 0x3FFD5F)
			{
				disk->type = FAT16;
				// Format to FAT16
				masterBootRecord->Partition0.PTE_FSDesc = 0x06;
				if (SectorWrite (0x00, gDataBuffer, TRUE) == FALSE)
					return EOF;

				FAT16DataClusters = secCount - 0x218;
				// Figure out how many sectors per cluster we need
				disk->SecPerClus = 1;
				while (FAT16DataClusters > 0xFFED)
				{
					disk->SecPerClus *= 2;
					FAT16DataClusters /= 2;
				}
				// This shouldnt happen
				if (disk->SecPerClus > 128)
					return EOF;

				// Prepare a boot sector
				memset (gDataBuffer, 0x00, MEDIA_SECTOR_SIZE);

				// Last digit of file system name (FAT16   )
				gDataBuffer[58] = '6';

			}
			else
			{
				// Cannot format; too many sectors
				return EOF;
			}

			// Calculate the size of the FAT
			fatsize = (secCount - 0x21  + (2*disk->SecPerClus));
			if (disk->type == FAT12)
				test =	(341 * disk->SecPerClus) + 2;
			else
				test = 	(256  * disk->SecPerClus) + 2;
			fatsize = (fatsize + (test-1)) / test;
			// Non-file system specific values	
			gDataBuffer[0] = 0xEB;			//Jump instruction
			gDataBuffer[1] = 0x3C;
			gDataBuffer[2] =  0x90;
			gDataBuffer[3] =  'M';			//OEM Name "MCHP FAT"
			gDataBuffer[4] =  'C';
			gDataBuffer[5] =  'H'; 
			gDataBuffer[6] =  'P';
			gDataBuffer[7] =  ' ';
			gDataBuffer[8] =  'F';
			gDataBuffer[9] =  'A';
			gDataBuffer[10] = 'T';
			gDataBuffer[11] =  0x00;	        //Bytes per sector - 512
			gDataBuffer[12] =  0x02;
			gDataBuffer[13] = disk->SecPerClus;	//Sectors per cluster
			gDataBuffer[14] = 0x08;			//Reserved sector count
			gDataBuffer[15] = 0x00;			
			disk->fat = 0x08 + disk->firsts;
			gDataBuffer[16] = 0x02;			//number of FATs
			disk->fatcopy = 0x02;
			gDataBuffer[17] = 0x00; 			//Max number of root directory entries - 512 files allowed
			gDataBuffer[18] = 0x02;
			disk->maxroot = 0x200;
			gDataBuffer[19] = 0x00;			//total sectors	
			gDataBuffer[20] = 0x00;
			gDataBuffer[21] = 0xF8;			//Media Descriptor
			gDataBuffer[22] = fatsize & 0xFF;			//Sectors per FAT
			gDataBuffer[23] = (fatsize >> 8) & 0xFF;
			disk->fatsize = fatsize;
			gDataBuffer[24] = 0x3F;	        //Sectors per track 
			gDataBuffer[25] = 0x00;
			gDataBuffer[26] = 0xFF;			//Number of heads 
			gDataBuffer[27] = 0x00;
			// Hidden sectors = sectors between the MBR and the boot sector
			gDataBuffer[28] = (BYTE)(disk->firsts & 0xFF);
			gDataBuffer[29] = (BYTE)((disk->firsts / 0x100) & 0xFF);
			gDataBuffer[30] = (BYTE)((disk->firsts / 0x10000) & 0xFF); 
			gDataBuffer[31] = (BYTE)((disk->firsts / 0x1000000) & 0xFF);
			// Total Sectors = same as sectors in the partition from MBR
			gDataBuffer[32] = (BYTE)(secCount & 0xFF);
			gDataBuffer[33] = (BYTE)((secCount / 0x100) & 0xFF);
			gDataBuffer[34] = (BYTE)((secCount / 0x10000) & 0xFF); 
			gDataBuffer[35] = (BYTE)((secCount / 0x1000000) & 0xFF);
			gDataBuffer[36] = 0x00;			// Physical drive number
			gDataBuffer[37] = 0x00;			// Reserved (current head) 
			gDataBuffer[38] = 0x29;			// Signature code
			gDataBuffer[39] = (BYTE)(serialNumber & 0xFF);
			gDataBuffer[40] = (BYTE)((serialNumber / 0x100) & 0xFF);
			gDataBuffer[41] = (BYTE)((serialNumber / 0x10000) & 0xFF);
			gDataBuffer[42] = (BYTE)((serialNumber / 0x1000000) & 0xFF);
			// Volume ID
			if (volumeID != NULL)
			{
				for (i = 0; (*(volumeID + i) != 0) && (i < 11); i++)
				{
					gDataBuffer[i + 43] = *(volumeID + i);
				}
				while (i < 11)
				{
					gDataBuffer[43 + i++] = 0x20;
				}
			}
			else
			{
				for (i = 0; i < 11; i++)
				{
					gDataBuffer[i+43] = 0;
				}
			}
			gDataBuffer[54] = 'F';
			gDataBuffer[55] = 'A';
			gDataBuffer[56] = 'T';
			gDataBuffer[57] = '1';
			gDataBuffer[59] = ' ';
			gDataBuffer[60] = ' ';
			gDataBuffer[61] = ' ';
#ifdef USE_PIC18
			// C18 can't reference a value greater than 256
			// using an array name pointer
			*(dataBufferPointer + 510) = 0x55;
			*(dataBufferPointer + 511) = 0xAA;
#else
			gDataBuffer[510] = 0x55;
			gDataBuffer[511] = 0xAA;
#endif			

	        disk->root = disk->fat + (disk->fatcopy * disk->fatsize);

			if (SectorWrite (disk->firsts, gDataBuffer, FALSE) == FALSE)
				return EOF;
			
			break;
		case 0:
			if (LoadBootSector (disk) != CE_GOOD)
				return EOF;
			break;
		default:
			return EOF;
	}

	// Erase the FAT
	memset (gDataBuffer, 0x00, MEDIA_SECTOR_SIZE);
	gDataBuffer[0] = 0xF8;
	gDataBuffer[1] = 0xFF;
	gDataBuffer[2] = 0xFF;
	if (disk->type == FAT16)
		gDataBuffer[3] = 0xFF;

	for (j = disk->fatcopy - 1; j != 0xFFFF; j--)
	{
		if (SectorWrite (disk->fat + (j * disk->fatsize), gDataBuffer, FALSE) == FALSE)
			return EOF;
	}
			
	memset (gDataBuffer, 0x00, 4);
	
	for (i = disk->fat + 1; i < (disk->fat + disk->fatsize); i++)
	{
		for (j = disk->fatcopy - 1; j != 0xFFFF; j--)
		{
			if (SectorWrite (i + (j * disk->fatsize), gDataBuffer, FALSE) == FALSE)
				return EOF;
		}
	}

	// Erase the root directory
	RootDirSectors = ((disk->maxroot * 32) + (0x200 - 1)) / 0x200;               

	for (i = 1; i < RootDirSectors; i++)
	{
		if (SectorWrite (disk->root + i, gDataBuffer, FALSE) == FALSE)
			return EOF;
	}

	if (volumeID != NULL)
	{
		// Create a drive name entry in the root dir
		i = 0;
		while ((*(volumeID + i) != 0) && (i < 11))
		{
			gDataBuffer[i] = *(volumeID + i);
			i++;
		}
		while (i < 11)
		{
			gDataBuffer[i++] = ' ';
		}
		gDataBuffer[11] = 0x08;
		gDataBuffer[17] = 0x11;
		gDataBuffer[19] = 0x11;
		gDataBuffer[23] = 0x11;
		
		if (SectorWrite (disk->root, gDataBuffer, FALSE) == FALSE)
			return EOF;
	}	
	else
	{
		if (SectorWrite (disk->root, gDataBuffer, FALSE) == FALSE)
			return EOF;
	}

	return 0;
}
#endif
#endif

/******************************************************************************
* Function:        BYTE Write_File_Entry( FILEOBJ fo, WORD * curEntry)
*
* PreCondition:    Disk mounted, fo contains valid file data
*
* Input:           fo         - File structure
*                  curEntry   - Write destination
*                  
* Output:         TRUE    - Operation successful
*                 FALSE   - Operation failed 
*
* Side Effects:    None
*
* Overview:        Write the buffer into the current entry
*
* Note:            None
*****************************************************************************/

#ifdef ALLOW_WRITES
BYTE Write_File_Entry( FILEOBJ fo, WORD * curEntry)
{
    DISK   *dsk;
    BYTE   status;
    BYTE   offset2;
    DWORD   sector;
    WORD   ccls;
    
    dsk = fo->dsk;
    
    // get the cluster of this entry
    ccls = fo->dirccls;
    
    offset2  = (*curEntry >> 4);
    
    // if its not the root, it's cluster based  
    if(ccls != 0)
        offset2 = offset2 % (dsk->SecPerClus);
    
    sector = Cluster2Sector(dsk,ccls);
    
    // Now write it       
    if ( !SectorWrite( sector + offset2, dsk->buffer, FALSE)) 
        status = FALSE;
    else
        status = TRUE;                      
    
    return(status);
} // Write_File_Entry 
#endif


/******************************************************************************
* Function:        BYTE FAT_erase_cluster_chain (WORD cluster, DISK * dsk)
*
* PreCondition:    Disk mounted, should be called from FILEerase
*
* Input:           cluster   - The cluster number
*                  dsk       - The disk structure
*                  
* Output:          TRUE      - Operation successful
*                  FALSE     - Operation failed 
*               
*
* Side Effects:    None
*
* Overview:        Erase the cluster chain
*
* Note:            None
*****************************************************************************/

#ifdef ALLOW_WRITES
BYTE FAT_erase_cluster_chain (WORD cluster, DISK * dsk)
{ 
    WORD     c,c2;
    enum    _status {Good, Fail, Exit}status;
    
    status = Good;
    
    // Make sure there is actually a cluster assigned
    if(cluster == 0 || cluster == 1)
    {
        status = Exit;
    }
    else
    {
        while(status == Good)
        {
            // Get the FAT entry
            if((c = ReadFAT( dsk, cluster)) == CLUSTER_FAIL)
                status = Fail;
            else
            {    
                if(c == 0 || c == 1)
                {
                    status = Exit;         
                }
                else
                {
                    // compare against max value of a cluster in FATxx
                    c2 = LAST_CLUSTER;
                    
                    // look for the last cluster in the chain
                    if ( c >= c2)    
                        status = Exit;
            
                    // Now erase this FAT entry
                    if(WriteFAT(dsk, cluster, CLUSTER_EMPTY, FALSE) == CLUSTER_FAIL)
                        status = Fail;
            
                    // now update what the current cluster is 
                    cluster = c;
                }
            }
        }// while status
    }// cluster == 0
    
    WriteFAT (dsk, 0, 0, TRUE);
    
    if(status == Exit)
        return(TRUE);
    else    
        return(FALSE);
} // Erase cluster
#endif

/******************************************************************************
* Function:        DIRENTRY Cache_File_Entry( FILEOBJ fo, WORD * curEntry, BYTE ForceRead)
*
* PreCondition:    Disk mounted
*
* Input:           fo          - File information
*                  curEntry    - Location of the sector
*                  ForceRead   - Forces loading of a new sector of the root
*                  
* Output:          DIRENTRY    - Sector loaded
*
* Side Effects:    None
*
* Overview:        Get the sector loaded as pointed to by curEntry
*
* Note:            Should not be called by user
*****************************************************************************/

DIRENTRY Cache_File_Entry( FILEOBJ fo, WORD * curEntry, BYTE ForceRead)
{
    DIRENTRY dir;
    DISK *dsk;
    DWORD sector;
    WORD cluster;
    WORD ccls;
    BYTE offset2;
    BYTE numofclus;
    
    dsk = fo->dsk;
    
    // get the base sector of this directory
    cluster = fo->dirclus;
    ccls = fo->dirccls;
    
    // figure out the offset from the base sector
    offset2  = (*curEntry >> 4);
    offset2 = offset2; // emulator issue
    
    // if its the root its not cluster based
    if(cluster != 0)  
        offset2  = offset2 % (dsk->SecPerClus);   // figure out the offset
    
    // check if a new sector of the root must be loaded
    if (ForceRead || (*curEntry & 0xf) == 0)    // only 16 entries per sector
    {
        // see if we have to load a new cluster
        if((offset2 == 0 && (*curEntry) >= DIRENTRIES_PER_SECTOR) || ForceRead)
        {
            if(cluster == 0)
            {
            ccls = 0;
            }
            else
            {
                // If ForceRead, read the number of sectors from 0
                if(ForceRead)
                    numofclus = ((WORD)(*curEntry) / (WORD)(((WORD)DIRENTRIES_PER_SECTOR) * (WORD)dsk->SecPerClus));
                // Otherwise just read the next sector
                else
                    numofclus = 1;
    
                // move to the correct cluster
                while(numofclus)
                {
                    ccls = ReadFAT(dsk, ccls);  
    
                    if(ccls >= LAST_CLUSTER)
                        break;
                    else          
                        numofclus--;
                }
            }   
        }       
    
        // see if that we have a valid cluster number
        if(ccls < LAST_CLUSTER)
        {           
            fo->dirccls = ccls; // write it back
    
            sector = Cluster2Sector(dsk,ccls);
    
            // see if we are root and about to go pass our boundaries
            if(ccls == 0 && (sector + offset2) >= dsk->data)
            {
                dir = ((DIRENTRY)NULL);   // reached the end of the root
            }
            else
            {
                #ifdef ALLOW_WRITES
                    if (gNeedDataWrite)
                        if (flushData())
                            return NULL;
                #endif
                gBufferOwner = NULL;
                gBufferZeroed = FALSE;
    
                if ( SectorRead( sector + offset2, dsk->buffer) != TRUE) 
                {
                    dir = ((DIRENTRY)NULL);
                }
                else
                {
                    if(ForceRead)
                        dir = (DIRENTRY)((DIRENTRY)dsk->buffer) + ((*curEntry)%DIRENTRIES_PER_SECTOR);
                    else
                        dir = (DIRENTRY)dsk->buffer;                        
                }
                gLastDataSectorRead = 0xFFFFFFFF;
            }
        }
        else
            dir = ((DIRENTRY)NULL);   
    }
    else
        dir = (DIRENTRY)((DIRENTRY)dsk->buffer) + ((*curEntry)%DIRENTRIES_PER_SECTOR);
    
    return(dir);
} // Cache_File_Entry 


/******************************************************************************
* Function:        CETYPE CreateFileEntry(FILEOBJ fo, WORD *fHandle)
*
* PreCondition:    File opened in WRITE mode
*
* Input:           fo           - Pointer to file structure
*                  fHandle      - Location to create file
*                  
* Output:         CE_GOOD          - File creation successful
*                 CE_DIR_FULL      - All root dir entry are taken
*
* Side Effects:    None
*
* Overview:        With the data passed within fo, create a new file entry in the current directory
*
* Note:            None
*****************************************************************************/

#ifdef ALLOW_WRITES
CETYPE CreateFileEntry(FILEOBJ fo, WORD *fHandle, BYTE mode)
{
    BYTE    index;
    CETYPE  error = CE_GOOD;
    char    name[11];
    
    for (index = 0; index < FILE_NAME_SIZE; index ++)
    {
        name[index] = fo->name[index];
    }
    
    // make sure we are good
    if(error == CE_GOOD)
    { 
        *fHandle = 0;
    
        // figure out where to put this file in the directory stucture 
        if(FindEmptyEntries(fo, fHandle))
        {       
            // found the entry, now populate it 
            if((error = PopulateEntries(fo, name ,fHandle, mode)) == CE_GOOD)
            {
                // if everything is ok, create a first cluster
                error = CreateFirstCluster(fo);                
            }
        }
        else
        {
            error = CE_DIR_FULL;
        }      
    }
    return(error);
}
#endif

/******************************************************************************
* Function:        CETYPE CreateFirstCluster(FILEOBJ fo)
*
* PreCondition:    Called from CreateFileEntry
*
* Input:           fo   - The file that contains the first cluster
*                  
* Output:          CE_GOOD          - First cluster created
*                  CE_WRITE_ERROR   - Cluster creation failed
*
* Side Effects:    None
*
* Overview:        Finds an open cluster and links it in
*
* Note:            Should not be called by user
*****************************************************************************/

#ifdef ALLOW_WRITES
CETYPE CreateFirstCluster(FILEOBJ fo)
{
    CETYPE    error;
    WORD      cluster;
    WORD      fHandle;
    DIRENTRY  dir;
    
    fHandle = fo->entry;
    
    // Now create the first cluster (head cluster)
    if((error = FILECreateHeadCluster(fo,&cluster)) == CE_GOOD)
    {        
        // load it so it can be linked in the new cluster
        dir = LoadDirAttrib(fo, &fHandle);
        
        // Now update the new cluster
        dir->DIR_FstClusLO = cluster;
        
        // now write it
        if(Write_File_Entry(fo, &fHandle) != TRUE)
            error = CE_WRITE_ERROR;
    } // Create Cluster   
    
    return(error);
}// End of CreateFirstCluster
#endif

/******************************************************************************
* Function:        BYTE FindEmptyEntries(FILEOBJ fo, WORD *fHandle)
*
* PreCondition:    Disk mounted, CreateFileEntry called
*
* Input:           fo           - Pointer to file structure
*                  fHandle      - Start of entries
*                  
* Output:          TRUE    - One found
*                  FALSE   - None found
*
* Side Effects:    None
*
* Overview:        Find the passed number of contingiant empty entries
*
* Note:            Should not be called by user
*****************************************************************************/

#ifdef ALLOW_WRITES
BYTE FindEmptyEntries(FILEOBJ fo, WORD *fHandle)
{
    BYTE        status = NOT_FOUND;
    BYTE        amountfound;
    BYTE        a;
    WORD        bHandle, b;
    DIRENTRY    dir;
    
    fo->dirccls = fo->dirclus;
    if((dir = Cache_File_Entry( fo, fHandle, TRUE)) == NULL)
    {
        status = CE_BADCACHEREAD;
    }
    else
    {
        // while its still not found
        while(status == NOT_FOUND)
        {
            amountfound = 0;
            bHandle = *fHandle;
    
            // find (number) continuous entries
            do
            {
                // Get the entry
                dir = Cache_File_Entry( fo, fHandle, FALSE);
                
                // Read the first char of the file name 
                a = dir->DIR_Name[0];     
                
                // increase number
                (*fHandle)++;
            }while((a == DIR_DEL || a == DIR_EMPTY) && (dir != (DIRENTRY)NULL) &&  (++amountfound < 1));  
    
            // --- now why did we exit?
            if(dir == NULL) // Last entry of the cluster
            {
                //setup the current cluster
                b = fo->dirccls; // write it back
                
                // make sure we are not the root directory
                if(b == 0)
                    status = NO_MORE;
                else    
                {
                fo->ccls = b;
                
                if(FILEallocate_new_cluster(fo, 1) == CE_DISK_FULL)
                    status = NO_MORE;    
                else
                {
                    *fHandle = bHandle;
                    status = FOUND;     // a new cluster will surely hold a new file name     
                }
            }
        }
        else
        {            
            if(amountfound == 1)
            {
                status = FOUND;
                *fHandle = bHandle;
            }
        }
    }// while    

    // copy the base handle over
    *fHandle = bHandle;
    }
    
    if(status == FOUND)
        return(TRUE);
    else
        return(FALSE);        
}
#endif

/******************************************************************************
* Function:        BYTE PopulateEntries(FILEOBJ fo, char *name , WORD *fHandle)
*
* PreCondition:    Disk mounted, CreateFileEntry called
*
* Input:           fo           - Pointer to file structure
*                  name         - Name of the file
*                  fHandle      - Location of the file
*                  
* Output:          CE_GOOD      - Population successful
*
* Side Effects:    None
*
* Overview:        Actually fill up the directory entry with the data at hand and write it  
*
* Note:            Should not be called by user
*****************************************************************************/

#ifdef ALLOW_WRITES
BYTE PopulateEntries(FILEOBJ fo, char *name , WORD *fHandle, BYTE mode)
{
    BYTE        error = CE_GOOD;
    DIRENTRY    dir;
    
    fo->dirccls = fo->dirclus;
    dir = Cache_File_Entry( fo, fHandle, TRUE);
    
    if (dir == NULL)
        return CE_BADCACHEREAD;
    
    // copy the contents over
    strncpy(dir->DIR_Name,name,DIR_NAMECOMP);
    
    // setup no attributes
    if (mode == DIRECTORY)
        dir->DIR_Attr = ATTR_DIRECTORY;
    else
        dir->DIR_Attr   = ATTR_ARCHIVE;
    
    dir->DIR_NTRes = 0x00; // nt reserved
    dir->DIR_FstClusHI = 0x00;    // high word of this entry's first cluster number
    dir->DIR_FstClusLO = 0x0000;    // low word of this entry's first cluster number
    dir->DIR_FileSize = 0x0;     // file size in DWORD    
    
    // Timing information for uncontrolled clock mode   
    #ifdef INCREMENTTIMESTAMP
        dir->DIR_CrtTimeTenth = 0xB2; // millisecond stamp
        dir->DIR_CrtTime = 0x7278;      // time created 
        dir->DIR_CrtDate = 0x32B0;      // date created 
        if (mode != DIRECTORY)
        {
            dir->DIR_LstAccDate = 0x32B0;      // Last Access date
            dir->DIR_WrtTime = 0x7279;      // last update time
            dir->DIR_WrtDate = 0x32B0;      // last update date
        }
        else
        {
            dir->DIR_LstAccDate = 0x0000;      // Last Access date
            dir->DIR_WrtTime = 0x0000;        // last update time
            dir->DIR_WrtDate = 0x0000;      // last update date
        }
    #endif
    
    #ifdef USEREALTIMECLOCK
        CacheTime();
        dir->DIR_CrtTimeTenth = gTimeCrtMS;         // millisecond stamp
        dir->DIR_CrtTime = gTimeCrtTime;      // time created //
        dir->DIR_CrtDate = gTimeCrtDate;      // date created (1/1/2004)
        if (mode != DIRECTORY)
        {
            dir->DIR_LstAccDate = gTimeAccDate;      // Last Access date
            dir->DIR_WrtTime = gTimeWrtTime;      // last update time
            dir->DIR_WrtDate = gTimeWrtDate;      // last update date
        }
        else
        {
            dir->DIR_LstAccDate = 0x0000;      // Last Access date
            dir->DIR_WrtTime = 0x0000;      // last update time
            dir->DIR_WrtDate = 0x0000;      // last update date
        }
    #endif
    
    #ifdef USERDEFINEDCLOCK
        // The user will have set the time before this funciton is called
        dir->DIR_CrtTimeTenth = gTimeCrtMS;
        dir->DIR_CrtTime = gTimeCrtTime;
        dir->DIR_CrtDate = gTimeCrtDate;
        if (mode != DIRECTORY)
        {
            dir->DIR_LstAccDate = gTimeAccDate;
            dir->DIR_WrtTime = gTimeWrtTime;
            dir->DIR_WrtDate = gTimeWrtDate;
        }
        else
        {
            dir->DIR_LstAccDate = 0x0000;      // Last Access date
            dir->DIR_WrtTime = 0x0000;      // last update time
            dir->DIR_WrtDate = 0x0000;      // last update date
        }
    #endif
    
    fo->size = dir->DIR_FileSize;
    fo->time = dir->DIR_CrtTime;
    fo->date = dir->DIR_CrtDate;
    fo->attributes = dir->DIR_Attr;
    fo->entry = *fHandle;
    
    // just write the last entry in
    Write_File_Entry(fo,fHandle);
    
    return(error);
}

#ifdef USEREALTIMECLOCK

/******************************************************************************
* Function:        void CacheTime (void)
*
* PreCondition:    RTCC module enabled
*
* Input:           None
*                  
* Output:          None
*
* Side Effects:    Modifies global timing variables
*
* Overview:        Puts values from RTCC into global timing variables  
*
* Note:            Should not be called by user
*****************************************************************************/

void CacheTime (void)
{
    WORD    year, monthday, weekhour, minsec, c, result;
    BYTE    ptr1, ptr0;
    
    if(RCFGCALbits.RTCPTR0)
        ptr0 = 1;
    else
        ptr0 = 0;
    if (RCFGCALbits.RTCPTR1)
        ptr1 = 1;
    else
        ptr1 = 0;
    
    RCFGCALbits.RTCPTR0 = 1;
    RCFGCALbits.RTCPTR1 = 1;
    year = RTCVAL;
    monthday = RTCVAL;
    weekhour = RTCVAL;
    minsec = RTCVAL;
    
    if (ptr0 == 1)
        RCFGCALbits.RTCPTR0 = 1;
    
    if (ptr1 == 1)
        RCFGCALbits.RTCPTR1 = 1;
    
    c = 0;
    c += (year & 0x0F);
    c += ((year & 0xF0) >> 4) * 10;
    // c equals the last 2 digits of the year from 2000 to 2099
    // Add 20 to adjust it to FAT time (from 1980 to 2107)
    c += 20;
    // shift the result to bits 
    result = c << 9;
    
    if ((monthday & 0x1000) == 0x1000)
    {
        c = 10;
    }
    else
    {
        c = 0;
    }
    c += ((monthday & 0x0F00) >> 8);
    c <<= 5;
    result |= c;
    
    c = (monthday & 0x00F0) >> 4;
    c *= 10;
    c += (monthday & 0x000F);
    
    result |= c;
    
    gTimeCrtDate = result;
    gTimeWrtDate = result;
    gTimeAccDate = result;
    
    c = ((weekhour & 0x00F0) >> 4) * 10;
    c += (weekhour & 0x000F); 
    result = c << 11;
    c = ((minsec & 0xF000) >> 12) * 10;
    c += (minsec & 0x0F00) >> 8;
    result |= (c << 5);
    c = ((minsec & 0x00F0) >> 4) * 10;
    c += (minsec & 0x000F);
    
    // If seconds mod 2 is 1, add 1000 ms
    if (c % 2)
        gTimeCrtMS = 100;
    else
        gTimeCrtMS = 0;
    
    c >>= 1;
    result |= c;
    
    gTimeCrtTime = result;
    gTimeWrtTime = result; 
}
#endif

#ifdef USERDEFINEDCLOCK

/*******************************************************************************************************************
* Function:        int SetClockVars (unsigned int year, unsigned char month, 
*                       unsigned char day, unsigned char hour, unsigned char minute, unsigned char second)
*
* PreCondition:    None
*
* Input:           unsigned int year      - The year (1980-2107)
*                  unsigned char month    - The month (1-12)
*                  unsigned char day      - The day of the month (1-31)
*                  unsigned char hour     - The hour (0-23)
*                  unsigned char minute   - The minute (0-59)
*                  unsigned char second   - The second (0-59)
*                  
* Output:          None
*
* Side Effects:    Modifies global timing variables
*
* Overview:        Lets the user manually set the timing variables
*
* Note:            Call this before creating a file or directory (set create time)
*                  and before closing a file (set last access time, last modified time)
******************************************************************************************************************/

int SetClockVars (unsigned int year, unsigned char month, unsigned char day, unsigned char hour, unsigned char minute, unsigned char second)
{
    unsigned int result;
    
    if ((year < 1980) || (year > 2107) || (month < 1) || (month > 12) ||
        (day < 1) || (day > 31) || (hour > 23) || (minute > 59) || (second > 59))
        return -1;
    
    result = (year - 1980) << 9;
    result |= (unsigned int)((unsigned int)month << 5);
    result |= (day);
    
    gTimeAccDate = result;
    gTimeCrtDate = result;
    gTimeWrtDate = result;
    
    result = ((unsigned int)hour << 11);
    result |= (unsigned int)((unsigned int)minute << 5);
    result |= (second/2);
    
    gTimeCrtTime = result;
    gTimeWrtTime = result;
    
    if (second % 2)
        gTimeCrtMS = 100;
    else
        gTimeCrtMS = 0;

    return 0;   
}
#endif

#endif

/******************************************************************************
* Function:        BYTE FILEallocate_new_cluster( FILEOBJ fo, BYTE mode)
*
* PreCondition:    Disk mounted
*
* Input:           fo              - Pointer to file structure
*                  mode            - 0 if allocating a file cluster
*                                    1 if allocating a dir cluster
*
* Output:          CE_GOOD         - Cluster allocated
*                  CE_DISK_FULL    - No clusters available
*
* Side Effects:    None
*
* Overview:        Allocate new cluster to a file
*
* Note:            Should not be called by user
*****************************************************************************/

#ifdef ALLOW_WRITES
BYTE FILEallocate_new_cluster( FILEOBJ fo, BYTE mode)
{
    DISK *  dsk;
    WORD    c,curcls;
    
    dsk = fo->dsk;   
    c = fo->ccls;
    
    // find the next empty cluster
    c = FATfindEmptyCluster(fo);
    if (c == 0)
        return CE_DISK_FULL;
    // mark the cluster as taken, and last in chain
    if(dsk->type == FAT12) 
        WriteFAT( dsk, c, LAST_CLUSTER_FAT12, FALSE);
    else
        WriteFAT( dsk, c, LAST_CLUSTER_FAT16, FALSE);
    
    // link current cluster to the new one
    curcls = fo->ccls;
    
    WriteFAT( dsk, curcls, c, FALSE);
    
    // update the FILE structure
    fo->ccls = c;
    
    // IF this is a dir, we need to erase the cluster
    // If it's a file, we can leave it- the file size
    // will limit the data we see to the data that's been
    // written
    if (mode == 1)
        return (EraseCluster(dsk, c));
    else
        return CE_GOOD;

} // allocate new cluster
#endif

/******************************************************************************
* Function:        WORD FATfindEmptyCluster(FILEOBJ fo)
*
* PreCondition:    Disk mounted
*
* Input:           fo      - Pointer to file structure
*                  
* Output:          c       - Cluster; 0 if failed
*
* Side Effects:    None
*
* Overview:        Find the next available cluster
*
* Note:            Should not be called by user
*****************************************************************************/

#ifdef ALLOW_WRITES
WORD FATfindEmptyCluster(FILEOBJ fo)
{    
    DISK *  disk;
    WORD    value=0x0,c,curcls;
    
    disk = fo->dsk;   
    c = fo->ccls;
    
    // just in case
    if(c < 2)
        c = 2;
    
    curcls = c;
    
    // sequentially scan through the FAT looking for an empty cluster
    while(c)
    {
        // look at its value    
        if ( (value = ReadFAT(disk, c)) == CLUSTER_FAIL)
        {
            c = 0;
            break;
        }
    
        // check if empty cluster found
        if (value == CLUSTER_EMPTY)
            break;   
    
        c++;    // check next cluster in FAT
        // check if reached last cluster in FAT, re-start from top
        if (c == END_CLUSTER || c >= disk->maxcls)
            c = 2;
    
        // check if full circle done, disk full
        if ( c == curcls)
        {
            c = 0;
            break;
        }
    }  // scanning for an empty cluster
    return(c);
}
#endif

/******************************************************************************
* Function:        int FSfclose(FSFILE *fo)
*
* PreCondition:    File opened
*
* Input:           fo      - Pointer to file structure
*                  
* Output:          0              - File closed successfully
*                  EOF            - Error closing the file
*
* Side Effects:    None
*
* Overview:        Close a file
*
* Note:
*****************************************************************************/

int FSfclose(FSFILE   *fo)
{
    WORD        fHandle;
    #ifndef FS_DYNAMIC_MEM
        WORD        fIndex;
    #endif
    int        error = 72;
    #ifdef ALLOW_WRITES
        DIRENTRY    dir;
    #endif
    
    fHandle = fo->entry;
    
    #ifdef ALLOW_WRITES
        if(fo->flags.write)
        {
            if (gNeedDataWrite)
                if (flushData())
                    return EOF;
    
            // Write the current FAT sector to the disk
            WriteFAT (fo->dsk, 0, 0, TRUE);
    
            // Get the file entry
            dir = LoadDirAttrib(fo, &fHandle);
            
            if (dir == NULL)
            {
                error = EOF;
                return error;
            }
    
            // update the time
            #ifdef INCREMENTTIMESTAMP
                IncrementTimeStamp(dir);
            #elif defined USERDEFINEDCLOCK
                dir->DIR_WrtTime = gTimeWrtTime;
                dir->DIR_WrtDate = gTimeWrtDate;
            #elif defined USEREALTIMECLOCK
                CacheTime();
                dir->DIR_WrtTime = gTimeWrtTime;
                dir->DIR_WrtDate = gTimeWrtDate;
            #endif
    
            Nop();
    
            dir->DIR_FileSize = fo->size;
            
            // just write the last entry in
            if(Write_File_Entry(fo,&fHandle))
                error = 0;
            else
                error = EOF;
    
            // it's now closed
            fo->flags.write = FALSE;
        }
    #endif
    
    #ifdef FS_DYNAMIC_MEM
        FS_free((unsigned char *)fo);
    #else
    
        for( fIndex = 0; fIndex < FS_MAX_FILES_OPEN; fIndex++ )
        {
            if( fo == &gFileArray[fIndex] )
            {
                gFileSlotOpen[fIndex] = TRUE;
                break;
            }
        }
    #endif
    
    // File opened in read mode
    if (error == 72)
        error = 0;
    
    return(error);
} // FSfclose




/******************************************************************************
* Function:        void IncrementTimeStamp(DIRENTRY dir)
*
* PreCondition:    File opened
*
* Input:           dir      - Pointer to directory structure
*                  
* Output:          None
*
* Side Effects:    None
*
* Overview:        Increment the time so we can keep track of the files
*
* Note:            None
*****************************************************************************/
#ifdef INCREMENTTIMESTAMP
void IncrementTimeStamp(DIRENTRY dir)
{
    BYTE          seconds;
    BYTE          minutes;
    BYTE          hours;
    
    BYTE          day;
    BYTE          month;
    BYTE          year;
    
    seconds = (dir->DIR_WrtTime & 0x1f);
    minutes = ((dir->DIR_WrtTime & 0x07E0) >> 5);
    hours   = ((dir->DIR_WrtTime & 0xF800) >> 11);
    
    day     = (dir->DIR_WrtDate & 0x1f);
    month   = ((dir->DIR_WrtDate & 0x01E0) >> 5);
    year    = ((dir->DIR_WrtDate & 0xFE00) >> 9);
    
    if(seconds < 29)
    {
        // Increment number of seconds by 2
        // This clock method isn't intended to be accurate anyway
        seconds++;
    }
    else
    {
        seconds = 0x00;
        
        if(minutes < 59)
        {
            minutes++; 
        }
        else
        {
            minutes = 0;
            
            if(hours < 23)
            {
                hours++;
            }
            else
            {
                hours = 0;
                if(day < 30)
                {
                    day++;
                }
                else
                {
                    day = 1;
                    
                    if(month < 12)
                    {
                        month++;
                    }
                    else
                    {
                        month = 1;
                        // new year
                        year++;
                        // This is only valid until 2107
                    }
                }
            }
        }
    }
    
    dir->DIR_WrtTime = (WORD)(seconds);
    dir->DIR_WrtTime |= ((WORD)(minutes) << 5);
    dir->DIR_WrtTime |= ((WORD)(hours) << 11);  
    
    dir->DIR_WrtDate = (WORD)(day);
    dir->DIR_WrtDate |= ((WORD)(month) << 5);
    dir->DIR_WrtDate |= ((WORD)(year) << 9);
}
#endif

/******************************************************************************
* Function:        BYTE Fill_File_Object(FILEOBJ fo, WORD *fHandle)
*
* PreCondition:    Disk mounted
*
* Input:           fo        - Pointer to file structure
*                  fHandle   - Passed member's location
*                  
* Output:          FOUND       - Operation successful
*                  NOT_FOUND   - Operation failed
*
* Side Effects:    None
*
* Overview:        Fill the passed file object with the passed member's (fHandle) information
*
* Note:            Should not be called by user
*****************************************************************************/

BYTE Fill_File_Object(FILEOBJ fo, WORD *fHandle)
{
    DIRENTRY    dir;
    BYTE        index, a;         
    BYTE        character;
    BYTE        status;   
    DWORD       temp;
    BYTE        test = 0;
    
    // Get the entry
    if (((*fHandle & 0xf) == 0) && (*fHandle != 0))
    {
        fo->dirccls = fo->dirclus;
        dir = Cache_File_Entry(fo, fHandle, TRUE);
    }
    else
    {
        dir = Cache_File_Entry (fo, fHandle, FALSE);
    }
    
    // Read the first char of the file name 
    a = dir->DIR_Name[0]; 
    
    // Make sure there is a directory left
    if(dir == (DIRENTRY)NULL || a == DIR_EMPTY)
    {                  
        status = NO_MORE;
    }
    else
    {
        // Check for empty or deleted directory
        if ( a == DIR_DEL)
            status = NOT_FOUND;
        else
        {
            // Get the attributes
            a = dir->DIR_Attr; 
    
    
            /* 8.3 File Name */
    
            // print the file name and extension                        
            for (index=0; index < DIR_NAMESIZE; index++)
            {
                character = dir->DIR_Name[index];
                character = (BYTE)toupper(character); 
                
                fo->name[test++] = character;
            }
    
            // Get the attributes
            a = dir->DIR_Attr; 
            
            // its possible to have an extension in a directory
            character = dir->DIR_Extension[0];
            
            
            // Get the file extension if its there
            for (index=0; index < DIR_EXTENSION; index++)
            {
                character = dir->DIR_Extension[index];
                character = (BYTE)toupper(character);   
                fo->name[test++] = character;
            }
    
            // done and done with the name
            //         fo->name[++test] = (BYTE)'\0';
            
            // Now store the identifier
            fo->entry = *fHandle;
            
            
            // see if we are still a good file
            a = dir->DIR_Name[0];
            
            if(a == DIR_DEL)
                status = NOT_FOUND;
            else
                status = FOUND;
    
            // Now store the size
            fo->size = (dir->DIR_FileSize);
            
            // Now store the cluster
            temp = (dir->DIR_FstClusHI);
            temp = temp << 16;
            temp |= dir->DIR_FstClusLO;
            fo->cluster = temp;
            
            // get the date and time      
            fo->time = (dir->DIR_WrtTime);
            fo->date = (dir->DIR_WrtDate);
            
            /// -Get and store the attributes
            a = dir->DIR_Attr;           
            fo->attributes = a;
    
        }// deleted directory  
    }// Ensure we are still good
    return(status);
} // Fill_File_Object


/******************************************************************************
* Function:        DIRENTRY LoadDirAttrib(FILEOBJ fo, WORD *fHandle)
*
* PreCondition:    Disk mounted
*
* Input:           fo         - Pointer to file structure
*                  fHandle    - Information location
*                  
* Output:          DIRENTRY   - The directory entry
*
* Side Effects:    None
*
* Overview:        Load the current sector for the filename base and return the direntry
*
* Note:            Should not be called by user
*****************************************************************************/

DIRENTRY LoadDirAttrib(FILEOBJ fo, WORD *fHandle)
{
    DIRENTRY    dir;
    BYTE      a;         
    
    fo->dirccls = fo->dirclus;
    // Get the entry
    dir = Cache_File_Entry( fo, fHandle, TRUE);
    if (dir == NULL)
        return NULL;   
    
    
    // Read the first char of the file name 
    a = dir->DIR_Name[0]; 
    
    // Make sure there is a directory left
    if(a == DIR_EMPTY)
        dir = (DIRENTRY)NULL;
    
    if(dir != (DIRENTRY)NULL)
    {
        // Check for empty or deleted directory
        if ( a == DIR_DEL)
            dir = (DIRENTRY)NULL;
        else
        {
            // Get the attributes
            a = dir->DIR_Attr; 
    
            // scan through all the long dir entries
            while(a == ATTR_LONG_NAME)
            {   
                (*fHandle)++;
                dir = Cache_File_Entry( fo, fHandle, FALSE);
                if (dir == NULL)
                    return NULL;
                a = dir->DIR_Attr;      
            } // long file name while loop
        } // deleted dir
    }// Ensure we are still good

    return(dir);
} // LoadDirAttrib


/******************************************************************************
* Function:        CETYPE FILEerase( FILEOBJ fo, WORD *fHandle, BYTE EraseClusters)
*
* PreCondition:    File not opened, File exists
*
* Input:           fo               - Pointer to file structure
*                  fHandle          - Location of file information
*                  EraseClusters    - Remove cluster allocation from FAT?
*                  
* Output:          CE_GOOD             - File erased successfully
*                  CE_FILE_NOT_FOUND   - Could not find the file on the card
*                  CE_ERASE_FAIL       - Internal Card erase failed
*
* Side Effects:    None
*
* Overview:        Erase file
*
* Note:            Should not be called by user
*****************************************************************************/

#ifdef ALLOW_WRITES
CETYPE FILEerase( FILEOBJ fo, WORD *fHandle, BYTE EraseClusters)
{
    DIRENTRY    dir;
    BYTE        a;           
    CETYPE      status = CE_GOOD;   
    WORD        clus;  
    DISK *      disk;
    
    disk = fo->dsk;    
    
    // reset the cluster
    clus = fo->dirclus;
    fo->dirccls = clus;
    
    // load the sector
    dir = Cache_File_Entry(fo, fHandle, TRUE);
    if (dir == NULL)
        return CE_BADCACHEREAD;
    
    // Fill up the File Object with the information pointed to by fHandle
    a = dir->DIR_Name[0]; 
    
    // see if there is something in the dir
    if(dir == (DIRENTRY)NULL || a == DIR_EMPTY)
    {                  
        status = CE_FILE_NOT_FOUND;
    }
    else
    {
        // Check for empty or deleted directory
        if ( a == DIR_DEL)
            status = CE_FILE_NOT_FOUND;
        else
        {        
            // Get the attributes
            a = dir->DIR_Attr; 
    
            /* 8.3 File Name - entry*/
            dir->DIR_Name[0] = DIR_DEL; // mark as deleted
            
            // Get the starting cluster
            clus = dir->DIR_FstClusLO;
    
            // Now write it 
            if(status != CE_GOOD || !(Write_File_Entry( fo, fHandle)))
                status = CE_ERASE_FAIL;
            else
            {
                if (clus != 0)
                {
                    if(EraseClusters)
                    {
                        /* Now remove the cluster allocation from the FAT */
                        status = ((FAT_erase_cluster_chain(clus, disk)) ? CE_GOOD : CE_ERASE_FAIL);    
                    }
                }
            }
        } // Not already deleted
    }// Not existant

    return (status);
    }
#endif


/******************************************************************************
* Function:        int FSrename (const rom char * fileName, FSFILE * fo)
*
* PreCondition:    None
*
* Input:           fileName   - The new name of the file
*                  fo         - The file to rename
*                  
* Output:          int        - Returns 0 if success, -1 otherwise
*
* Side Effects:    None
*
* Overview:        Change the name of a file or directory
*
* Note:            None
*****************************************************************************/

#ifdef ALLOW_WRITES

int FSrename (const char * fileName, FSFILE * fo)
{
    unsigned char j, k = 0;
    char string[12];
    WORD fHandle = 1, goodHandle;
    DIRENTRY    dir;

#ifdef ALLOW_DIRS
    WORD        dirclus;
    DWORD       sector;
    BYTE        offset2;

    if (fo == NULL)
    {
        // If fo == NULL, rename the CWD
    
        // You can't rename the root directory
        if (cwdptr->dirclus == 0)
            return -1;
    
        for (j = 0; (j < 11) && (*(fileName + j) != 0); j++)
        {
            string[j] = *(fileName + j);
        }
        while (j < 11)
        {
            string[j++] = 0x20;
        }
    
        // Make sure the dir name is valid
        FormatDirName (string, 0);
        // Get the cluster address of the CWD
        dirclus = cwdptr->dirclus;
        cwdptr->dirccls = cwdptr->dirclus;
        // Load the dotdot entry
        dir = Cache_File_Entry (cwdptr, &fHandle, TRUE);
        // Change the directory cluster of the CWD to point to the previous dir
        cwdptr->dirccls = dir->DIR_FstClusLO;
        cwdptr->dirclus = dir->DIR_FstClusLO;
    
        // Load the first entry of the previous dir
        // Start at 0 in case it's the root
        fHandle = 0;
        dir = Cache_File_Entry (cwdptr, &fHandle, TRUE);
        if (dir == NULL)
            return -1;
        // Check if the file name is already used
        for (j = 0; j < 11; j++)
        {
            if (dir->DIR_Name[j] != string[j])
                k = 1;
        }
        if (k == 0)
            return -1;
        else
            k = 0;
        // Look through it until we find the cluster to rename
        while ((dir->DIR_FstClusLO != dirclus) || 
            ((dir->DIR_FstClusLO == dirclus) && 
            (((unsigned char)dir->DIR_Name[0] == 0xE5) || (dir->DIR_Attr == ATTR_VOLUME) || (dir->DIR_Attr == ATTR_LONG_NAME)))) 
        {
            // Look through the entries until we get the
            // right one
            dir = Cache_File_Entry (cwdptr, &fHandle, FALSE);
            if (dir == NULL)
                return -1;
            for (j = 0; j < 11; j++)
            {
                if (dir->DIR_Name[j] != string[j])
                    k = 1;
            }
            if (k == 0)
                return -1;
            else
                k = 0;
            fHandle++;
        }
        fHandle--;
        goodHandle = fHandle++;
        // Keep looking through the directory to make sure none of the
        // other files have the same name
        while (1)
        {
            // Look through the entries until we get to the end
            // to make sure the name isn't taken
            dir = Cache_File_Entry (cwdptr, &fHandle, FALSE);
            if (dir == NULL)
                return -1;
            if (dir->DIR_Name[0] == 0)
                break;
            for (j = 0; j < 11; j++)
            {
                if (dir->DIR_Name[j] != string[j])
                    k = 1;
            }
            if (k == 0)
                return -1;
            else
                k = 0;
            fHandle++;
        }
    
        fHandle = goodHandle;
        // Reload the correct entry
        cwdptr->dirccls = cwdptr->dirclus;
        dir = Cache_File_Entry (cwdptr, &fHandle, TRUE);
    
        // Found it- now copy the name
        for (j = 0; j < 11; j++)
        {
            cwdptr->name[j] = string[j];
            dir->DIR_Name[j] = string[j];
        }
    
        // Figure out the base cluster for this directory
        sector = Cluster2Sector (cwdptr->dsk, cwdptr->dirccls);
        // Figure how how many sectors we've gone into the dir
        offset2  = (fHandle >> 4);
        // If it's not the root dir, it's cluster based
        if(cwdptr->dirclus != 0)
            offset2 = offset2 % (cwdptr->dsk->SecPerClus);
    
        if (SectorWrite((sector + offset2), cwdptr->dsk->buffer, FALSE) == FALSE)
        {
            return -1;
        }
    
        j = 0;
        while ((string[j] != 0x20) && (j < 11))
            j++;
    
        string[j] = 0x00;
    
        FSchdir (string);
    }
    else
    {
#else
    // If directories are disabled, ignore a NULL pointer
    if (fo == NULL)
        return -1;
#endif
    // If fo != NULL, rename the file
    if (FormatFileName (fileName, fo->name, 0) == FALSE)
    {
        return -1;
    }
    else
    {
        for (j = 0; j < 11; j++)
        {
            string[j] = fo->name[j];
        }
        goodHandle = fo->entry;
    
        fHandle = 0;
        fo->dirccls = fo->dirclus;
        dir = Cache_File_Entry (fo, &fHandle, TRUE);
        if (dir == NULL)
            return -1;
        // Check if the file name is already used
        for (j = 0; j < 11; j++)
        {
            if (dir->DIR_Name[j] != string[j])
            k = 1;
        }
        if (k == 0)
            return -1;
        else
            k = 0;
    
        while (1)
        {
            // Look through the entries until we get to the end
            // to make sure the name isn't taken
            dir = Cache_File_Entry (fo, &fHandle, FALSE);
            if (dir == NULL)
                return -1;
            if (dir->DIR_Name[0] == 0)
                break;
            for (j = 0; j < 11; j++)
            {
                if (dir->DIR_Name[j] != string[j])
                    k = 1;
            }
            if (k == 0)
                return -1;
            else
                k = 0;
            fHandle++;
        }
    
        fHandle = goodHandle;
        fo->dirccls = fo->dirclus;
    
        // Get the file entry
        dir = LoadDirAttrib(fo, &fHandle);
    
        if (dir == NULL)
        {
            return -1;
        }
    
        for (j = 0; j < 11; j++)
        {
            dir->DIR_Name[j] = fo->name[j];
        }
    
        // just write the last entry in
        if(!Write_File_Entry(fo,&fHandle))
            return -1;
    }
    
    #ifdef ALLOW_DIRS
    } // fo == NULL
    #endif
    
    return 0;
}

#endif // Allow writes

/******************************************************************************
* Function:        FSFILE * FSfopen (const char * fileName, const char *mode)
*
* PreCondition:    For read mode, file exists; FSInit called
*
* Input:           fileName   - The name of the file to open
*                  mode       - WRITE - Create a new file or replace an existing file
*                               READ - Read data from an existing file
*                               APPEND - Append data to an existing file
*                  
* Output:          FSFILE *     - The pointer to the file object
*
* Side Effects:    None
*
* Overview:        File open function
*
* Note:            None
*****************************************************************************/

FSFILE * FSfopen( const char * fileName, const char *mode )
{
    FILEOBJ     filePtr;
    #ifndef FS_DYNAMIC_MEM
        int         fIndex;
    #endif
    FSFILE      gblFileTemp;
    BYTE        ModeC;
    WORD        fHandle;
    CETYPE      final;
    
    if (WriteProtectState())
    {
        return NULL;
    }
    
    #ifdef FS_DYNAMIC_MEM
        filePtr = (FILEOBJ) FS_malloc(sizeof(FSFILE));
    #else
    
        filePtr = NULL;
        
        //Pick available file structure
        for( fIndex = 0; fIndex < FS_MAX_FILES_OPEN; fIndex++ )
        {
            if( gFileSlotOpen[fIndex] )   //this slot is available
            {
                gFileSlotOpen[fIndex] = FALSE;
                filePtr = &gFileArray[fIndex];
                break;
            }
        }
        
        if( filePtr == NULL )
            return NULL;      //no file structure slot available
    #endif
    
    //Format the source string.
    if( !FormatFileName(fileName, filePtr->name, 0) )
    {
        #ifdef FS_DYNAMIC_MEM
            FS_free( (unsigned char *)filePtr );
        #else
            gFileSlotOpen[fIndex] = TRUE;   //put this slot back to the pool
        #endif
        return NULL;   //bad filename
    }
    
    //Read the mode character
    ModeC = mode[0];
    
    filePtr->dsk = &gDiskData;
    filePtr->cluster = 0;
    filePtr->ccls    = 0;
    filePtr->entry = 0;
    filePtr->attributes = ATTR_ARCHIVE;
    
    // start at the current directory
    #ifdef ALLOW_DIRS
        filePtr->dirclus    = cwdptr->dirclus;
        filePtr->dirccls    = cwdptr->dirccls;
    #else
        filePtr->dirclus = 0;
        filePtr->dirccls = 0;
    #endif
    
    // copy file object over
    FileObjectCopy(&gblFileTemp, filePtr);
    
    // See if the file is found
    if(FILEfind (filePtr, &gblFileTemp, 1, 0) == CE_GOOD)
    {
        // File is Found
        switch(ModeC)
        {
            #ifdef ALLOW_WRITES
                case 'w':
                case 'W':
                {
                    // File exists, we want to create a new one, remove it first
                    fHandle = filePtr->entry;
                    final = FILEerase(filePtr, &fHandle, TRUE);
                    
                    if (final == CE_GOOD)
                    {
                        // now create a new one
                        final = CreateFileEntry (filePtr, &fHandle, 0);
                    
                        if (final == CE_GOOD)
                        {
                            final = FILEopen (filePtr, &fHandle, 'w');
                    
                            if (final == CE_GOOD)
                            {
                                final = FSfseek (filePtr, 0, SEEK_END);
                            }
                        }
                    }
                    break;
                }
    
                case 'A':
                case 'a':
                {
                    if(filePtr->size != 0)
                    {
                        fHandle = filePtr->entry;
                        final = FILEopen (filePtr, &fHandle, 'w');

                        if (final == CE_GOOD)
                        {
                            final = FSfseek (filePtr, 0, SEEK_END);
                        }
                    }
                    else
                    {
                        fHandle = filePtr->entry;
                        final = FILEerase(filePtr, &fHandle, TRUE);

                        if (final == CE_GOOD)
                        {
                            // now create a new one
                            final = CreateFileEntry (filePtr, &fHandle, 0);

                            if (final == CE_GOOD)
                            {
                                final = FILEopen (filePtr, &fHandle, 'w');

                                if (final == CE_GOOD)
                                {
                                    final = FSfseek (filePtr, 0, SEEK_END);
                                }
                            }
                        }
                    }
                    break;
                }
            #endif
            case 'R':
            case 'r':
            {
                fHandle = filePtr->entry;

                final = FILEopen (filePtr, &fHandle, 'r');

                break;
            }
    
            default:
                final = 0xFF;;  //indicate error condition
                break;
        }
    }
    else
    {
        #ifdef ALLOW_WRITES
            // the file was not found, reset to the default asked
            FileObjectCopy(filePtr, &gblFileTemp);

            // File is not Found
            if(ModeC == 'w' || ModeC == 'W' || ModeC == 'a' || ModeC == 'A')
            {
                // use the user requested name
                fHandle = 0;
                final = CreateFileEntry (filePtr, &fHandle, 0);
    
                if (final == CE_GOOD)
                {
                    final = FILEopen (filePtr, &fHandle, 'w');

                    if (final == CE_GOOD)
                    {
                        final = FSfseek (filePtr, 0, SEEK_END);
                    }
                }
            }
            else 
        #endif
                final = CE_FILE_NOT_FOUND;
    }

    #ifdef FS_DYNAMIC_MEM
        if( final != CE_GOOD )
        {
            FS_free( (unsigned char *)filePtr );
            filePtr = NULL;
        }
    #else
        if( final != CE_GOOD )
        {
            gFileSlotOpen[fIndex] = TRUE;   //put this slot back to the pool
            filePtr = NULL;
        }
    #endif

    return filePtr;
}

/******************************************************************************
* Function:        long FSftell (FSFILE * fo)
*
* PreCondition:    File opened
*
* Input:           fo         - Pointer to file structure
*                  
* Output:          fo->seek   - Current location of the file
*
* Side Effects:    None
*
* Overview:        User called function to determine the current location of a file
*
* Note:            None
*****************************************************************************/
long FSftell (FSFILE * fo)
{
    return (fo->seek);
}

/******************************************************************************
* Function:        int FSremove (const char * fileName)
*
* PreCondition:    File not opened, file exists
*
* Input:           compare   - Pointer to file to be erased
*                  
* Output:          0         - File removed
*                  1         - File was not removed
*
* Side Effects:    None
*
* Overview:        User called function to erase a file
*
* Note:            None
*****************************************************************************/

#ifdef ALLOW_WRITES
int FSremove (const char * fileName)
{
    FSFILE  f;
    FSFILE  gblFileTemp;
    FILEOBJ fo = &f;
    CETYPE  result;

    if (WriteProtectState())
    {
        return (-1);
    }

    //Format the source string
    if( !FormatFileName(fileName, fo->name, 0) )
        return -1;

    fo->dsk = &gDiskData;
    fo->cluster = 0;
    fo->ccls    = 0;
    fo->entry = 0;
    fo->attributes = ATTR_ARCHIVE;
    
    #ifndef ALLOW_DIRS
        // start at the root directory
        fo->dirclus    = 0;
        fo->dirccls    = 0;
    #else
        fo->dirclus = cwdptr->dirclus;
        fo->dirccls = cwdptr->dirccls;
    #endif
    
    // copy file object over
    FileObjectCopy(&gblFileTemp, fo);
    
    // See if the file is found
    result = FILEfind (fo, &gblFileTemp, 1, 0);
    
    if (result != CE_GOOD)
        return -1;
    result = FILEerase(fo, &fo->entry, TRUE);
    if( result == CE_GOOD )
        return 0;
    else
        return -1;
}
#endif

/******************************************************************************
* Function:        void FSrewind (FSFILE * fo)
*
* PreCondition:    File opened, file exists
*
* Input:           fo      - Pointer to file structure
*                  
* Output:          void
*
* Side Effects:    None
*
* Overview:        User called function to reset the position of the file
*
* Note:            None
*****************************************************************************/

void FSrewind (FSFILE * fo)
{
    #ifdef ALLOW_WRITES
        if (gNeedDataWrite)
            flushData();
    #endif
    fo->seek = 0;
    fo->pos = 0;
    fo->sec = 0;
    fo->ccls = fo->cluster;
    gBufferOwner = NULL;
    return;
}

/******************************************************************************
* Function:        void FileObjectCopy(FILEOBJ foDest,FILEOBJ foSource)
*
*
* Input:           foDest      - The destination
*                  foSource   - the source
*                  
* Output:          None
*
* Side Effects:    None
*
* Overview:        Makes a replica of an existing file object
*
* Note:            None
*****************************************************************************/

void FileObjectCopy(FILEOBJ foDest,FILEOBJ foSource)
{
    BYTE    i;
    BYTE    size;
    BYTE *  dest;
    BYTE *  source;
    
    dest = (BYTE *)foDest;
    source = (BYTE *)foSource;
    
    size = sizeof(FSFILE);
    
    for(i=0; i< size; i++)
    {
        dest[i] = source[i];    
    }
}

/******************************************************************************
* Function:        CETYPE FILECreateHeadCluster( FILEOBJ fo, WORD *cluster)
*
* PreCondition:    Disk mounted
*
* Input:           fo        - Pointer to file structure
*                  cluster   - Cluster location
*                  
* Output:         CE_GOOD            - File closed successfully
*                 CE_WRITE_ERROR     - Could not write to the sector
*                 CE_DISK_FULL       - All clusters in partition are taken
*
* Side Effects:    None
*
* Overview:        Use to create the HEAD cluster (1st one)
*
* Note:            Should not be called by user
*****************************************************************************/

#ifdef ALLOW_WRITES
CETYPE FILECreateHeadCluster( FILEOBJ fo, WORD *cluster)
{
    DISK *  disk;
    CETYPE  error = CE_GOOD;

    disk = fo->dsk;   
    
    // find the next empty cluster
    *cluster = FATfindEmptyCluster(fo);
    
    if(*cluster == 0)
        error = CE_DISK_FULL;
    else
    {    
        // mark the cluster as taken, and last in chain
        if(disk->type == FAT12) 
        {
            if(WriteFAT( disk, *cluster, LAST_CLUSTER_FAT12, FALSE) == CLUSTER_FAIL)
                error = CE_WRITE_ERROR;
        }
        else
        {        
            if(WriteFAT( disk, *cluster, LAST_CLUSTER_FAT16, FALSE) == CLUSTER_FAIL)
                error = CE_WRITE_ERROR;
        }
    
        // lets erase this cluster                
        if(error == CE_GOOD)
        {
            error = EraseCluster(disk,*cluster);
        }                
    }    
    return(error);
} // allocate head cluster
#endif

/******************************************************************************
* Function:        BYTE EraseCluster(DISK *disk, WORD cluster)
*
* PreCondition:    File opened
*
* Input:           dsk       - Disk structure
*                  cluster   - Cluster to be erased
*                  
* Output:          CE_GOOD             - File closed successfully
*                  CE_WRITE_ERROR      - Could not write to the sector
*
* Side Effects:    None
*
* Overview:        Erase the passed cluster
*
* Note:            Should not be called by user
*****************************************************************************/

#ifdef ALLOW_WRITES
BYTE EraseCluster(DISK *disk, WORD cluster)
{
    BYTE index;
    DWORD SectorAddress;
    BYTE error = CE_GOOD;

    SectorAddress = Cluster2Sector(disk,cluster);
    if (gNeedDataWrite)
        if (flushData())
            return EOF;

    gBufferOwner = NULL;

    if (gBufferZeroed == FALSE)
    {
        // clear out the memory first
        memset(disk->buffer, 0x00, MEDIA_SECTOR_SIZE);
        gBufferZeroed = TRUE;   
    }

    // Now clear them out
    for(index = 0; index < disk->SecPerClus && error == CE_GOOD; index++)
    {
        if (SectorWrite( SectorAddress++, disk->buffer, FALSE) != TRUE) 
            error = CE_WRITE_ERROR;
    }
    return(error);
}
#endif

/******************************************************************************
* Function:        DWORD Cluster2Sector(DISK * dsk, WORD cluster)
*
* PreCondition:    Disk mounted
*
* Input:           disk      - Disk structure
*                  cluster   - Cluster to be converted
*                  
* Output:          sector    - Sector that corresponds to given cluster
*
* Side Effects:    None
*
* Overview:        Given a cluster, figure out the sector number
*
* Note:            Should not be called by user
*****************************************************************************/

DWORD Cluster2Sector(DISK * dsk, WORD cluster)
{
    DWORD sector;
    
    // The root dir takes up cluster 0 and 1 
    if(cluster == 0 ||cluster == 1)
        sector = dsk->root + cluster;
    else
        sector = (((DWORD)cluster-2) * dsk->SecPerClus) + dsk->data;
    return(sector);
}

/******************************************************************************
* Function:        size_t FSfwrite(const void *ptr, size_t size, size_t n, FSFILE *stream)
*
* PreCondition:    File opened in WRITE mode
*
* Input:           stream      - Pointer to file structure
*                  ptr         - Pointer to source buffer
*                  n           - Number of units to transfer
*                  size        - Size of units in bytes
*
* Output:          size_t      - number of units written
*
* Side Effects:    None
*
* Overview:        Write file
*
* Note:            None
*****************************************************************************/

#ifdef ALLOW_WRITES
size_t FSfwrite(const void *ptr, size_t size, size_t n, FSFILE *stream)
{
    DWORD       count = size * n;
    BYTE   *    src = (BYTE *) ptr;
    DISK   *    dsk;                 // pointer to disk structure
    CETYPE      error = CE_GOOD;
    WORD        pos;
    DWORD       l;                     // absolute lba of sector to load
    DWORD       seek, filesize;
    WORD        writeCount = 0;
    
    // see if the file was opened in a write mode 
    if(!(stream->flags.write))
    {
        error = CE_WRITE_ERROR;
        return 0;
    }
    
    if (count == 0)   
        return 0;
    
    if (WriteProtectState())
    {
        error = CE_WRITE_PROTECTED;
        return 0;
    }
    
    gBufferZeroed = FALSE;
    
    dsk = stream->dsk;   
    
    // get the stated position
    pos = stream->pos;
    seek = stream->seek;
    
    l = Cluster2Sector(dsk,stream->ccls);     
    l += (WORD)stream->sec;      // add the sector number to it
    
    
    // Check if the current stream was the last one to use the
    // buffer. If not, check if we need to write data from the
    // old stream
    if (gBufferOwner != stream)
    {
        if (gNeedDataWrite)
        {
            if (flushData())
                return 0;
        }
        gBufferOwner = stream;
    }
    if (gLastDataSectorRead != l)
    {
        if (gNeedDataWrite)
        {
            if (flushData())
                return 0;
        }
    
        gBufferZeroed = FALSE;
        if(!SectorRead( l, dsk->buffer) )
            error = CE_BAD_SECTOR_READ;
        gLastDataSectorRead = l;
    }   
    // exit loop if EOF reached 
    filesize = stream->size;
    
    // 2. loop reading count bytes 
    while (error == CE_GOOD && count > 0)
    {          
        if( seek == filesize )
            stream->flags.FileWriteEOF = TRUE;
    
        // load a new sector if necessary, multiples of sector
        if (pos == MEDIA_SECTOR_SIZE) 
        {    
            BYTE needRead = TRUE;
    
            if (gNeedDataWrite)
                if (flushData())
                    return EOF;
    
            // reset position
            pos = 0;
    
            // point to the next sector
            stream->sec++;
            
            // get a new cluster if necessary            
            if (stream->sec == dsk->SecPerClus)
            {
                stream->sec = 0;
    
                if(stream->flags.FileWriteEOF)
                {
                    error = FILEallocate_new_cluster(stream, 0);    // add new cluster to the file
                    needRead = FALSE;
                }
                else
                    error = FILEget_next_cluster( stream, 1);                 
            }
    
            if (error == CE_DISK_FULL)
                return 0;                 

            if(error == CE_GOOD)
            {
                l = Cluster2Sector(dsk,stream->ccls);
                l += (WORD)stream->sec;      // add the sector number to it
                gBufferOwner = stream;
                // If we just allocated a new cluster, then the cluster will
                // contain garbage data, so it doesn't matter what we write to it
                // Whatever is in the buffer will work fine
                if (needRead)
                {
                    if( !SectorRead( l, dsk->buffer) )
                    {
                        error = CE_BAD_SECTOR_READ;
                        gLastDataSectorRead = 0xFFFFFFFF;
                        return 0;
                    }
                    else
                    {
                        gLastDataSectorRead = l;
                    }
                }
                else
                    gLastDataSectorRead = l;
            }
        } //  load new sector
    
        if(error == CE_GOOD)
        {
            // Write one byte at a time
            RAMwrite(dsk->buffer, pos++, *(char *)src);
            src = src + 1; // compiler bug
            seek++;
            count--;
            writeCount++;
            // now increment the size of the part
            if(stream->flags.FileWriteEOF)
                filesize++;             
            gNeedDataWrite = TRUE;
        }
    } // while count
    
    // save off the positon
    stream->pos = pos;
    
    // save off the seek
    stream->seek = seek;
    
    // now the new size
    stream->size = filesize;
    
    return(writeCount / size);
} // fwrite
#endif


/******************************************************************************
* Function:        BYTE flushData (void)
*
* PreCondition:    File opened in WRITE mode, data needs to be written
*
* Input:           None
*
* Output:          BYTE - returns CE_GOOD if data was written successfully
*
* Side Effects:    None
*
* Overview:        Writes an unwritten buffer to the card if necessary
*
* Note:            None
*****************************************************************************/

#ifdef ALLOW_WRITES
BYTE flushData (void)
{
    DWORD l;
    DISK * dsk;
    CETYPE error = CE_GOOD;
    
    // This will either be the pointer to the last file, or the handle
    FILEOBJ stream = gBufferOwner;
    
    dsk = stream->dsk;
    
    // figure out the lba
    l = Cluster2Sector(dsk,stream->ccls);
    
    l += (WORD)stream->sec;      // add the sector number to it
    
    if(!SectorWrite( l, dsk->buffer, FALSE))
    {
        error = CE_WRITE_ERROR;
        return EOF;
    } 
    
    gNeedDataWrite = FALSE;
    
    return 0;
}
#endif

/******************************************************************************
* Function:        int FSfeof( FSFILE * stream )
*
* PreCondition:    File is open is read mode
*
* Input:           stream        - Pointer to the target file
*
* Output:          NON Zero      - EOF reached
*                  Zero          - Not at end of File
*
* Side Effects:    None
*
* Overview:        Indicates that the current file pos is at the end
*
* Note:            None
*****************************************************************************/

int FSfeof( FSFILE * stream )
{
    return( stream->seek == stream->size );
}


/******************************************************************************
* Function:        size_t FSfread(void *ptr, size_t size, size_t n, FSFILE *stream)
*
* PreCondition:    FSfopen in the read mode has been executed
*
* Input:           stream      - File to be read
*                  ptr         - Destination buffer for read bytes
*                  n           - Number of units to be read
*                  size        - Size of units in bytes
*
* Output:          size_t      - number of units read
*
*
* Side Effects:    None
*
* Overview:        Reads data from a file into destination buffer
*
* Note:            None
*****************************************************************************/

size_t FSfread (void *ptr, size_t size, size_t n, FSFILE *stream)
{
    DWORD   len = size * n;
    BYTE *  pointer = (BYTE *) ptr;
    DISK *  dsk;               // Disk structure
    DWORD   seek, sec_sel;
    WORD    pos;       //position within sector
    CETYPE  error = CE_GOOD;
    WORD    readCount = 0;   
    
    dsk = (DISK *)stream->dsk;
    pos = stream->pos;
    seek = stream->seek;
    
    if( stream->flags.write )
    {
        return 0;   // CE_WRITEONLY
    }
    
    #ifdef ALLOW_WRITES
        if (gNeedDataWrite)
            if (flushData())
                return EOF;
    #endif
    
    // if it not my buffer, then get it from the disk.
    if( (gBufferOwner != stream) && (pos != MEDIA_SECTOR_SIZE ))
    {
        gBufferOwner = stream;
        sec_sel = Cluster2Sector(dsk,stream->ccls);
        sec_sel += (WORD)stream->sec;      // add the sector number to it
        
        gBufferZeroed = FALSE;
        if( !SectorRead( sec_sel, dsk->buffer) )
        {
            error = CE_BAD_SECTOR_READ;
            return 0;
        }
        gLastDataSectorRead = sec_sel;
    }
    
    //loop reading (count) bytes
    while( len )
    {
        if( seek == stream->size )
        {   
            error = CE_EOF;
            break;    
        }
    
        // In fopen, pos is init to 0 and the sect is loaded
        if( pos == MEDIA_SECTOR_SIZE )
        {
            // reset position
            pos = 0;
    
            // point to the next sector
            stream->sec++;
            
            // get a new cluster if necessary
            if( stream->sec == dsk->SecPerClus )
            {
                stream->sec = 0;
                if( (error = FILEget_next_cluster( stream, 1)) != CE_GOOD )
                    break;
            }
    
            sec_sel = Cluster2Sector(dsk,stream->ccls);
            sec_sel += (WORD)stream->sec;      // add the sector number to it
    
            gBufferOwner = stream;
            gBufferZeroed = FALSE;
            if( !SectorRead( sec_sel, dsk->buffer) )
            {   
                error = CE_BAD_SECTOR_READ;
                break; 
            }
            gLastDataSectorRead = sec_sel;
        }
    
        // copy one byte at a time
        *pointer = RAMread( dsk->buffer, pos++ );
        pointer++;
        seek++;
        readCount++;
        len--;
    }
    
    // save off the positon
    stream->pos = pos;
    
    // save off the seek
    stream->seek = seek;
    
    return(readCount / size);
} // fread


/******************************************************************************
* Function:        BYTE FormatFileName( const char* fileName, char* fN2, BYTE mode )
*
* PreCondition:    None
*
* Input:           fileName   - The name to be formatted
*                  fN2        - The location the formatted name will be stored
*               mode       - Non-zero if '*' are allowed
*
* Output:          TRUE       - Name formatted successfully
*                  FALSE      - File name could not be formatted
*
* Side Effects:    None
*
* Overview:        Format an 8.3 filename into FILE structure format
*
* Note:            None
*****************************************************************************/
BYTE FormatFileName( const char* fileName, char* fN2, BYTE mode)
{
    char *  pExt;
    WORD    temp;
    char    szName[15];
    BYTE    count;
    
    for (count = 0; count < 11; count++)
    {
        *(fN2 + count) = ' ';
    }
    
    // Make sure we dont have an empty string or a name with only
    // an extension
    if (fileName[0] == '.' || fileName[0] == 0)
        return FALSE;
    
    temp = strlen( fileName );
    
    if( temp <= 12 ) //8+3+1
        strcpy( szName, fileName );  //copy to RAM in case fileName is located in flash
    else
        return FALSE; //long file name
    
    // Make sure the characters are valid
    if ( !ValidateChars(szName, PICKFILENAME, mode) )
        return FALSE;
    
    //Look for '.' in the szName
    if( (pExt = strchr( szName, '.' )) != 0 )
    {
        *pExt = 0;
        pExt++; // now pointing to extension
    
        if( strlen( pExt ) > 3 ) // make sure the extension is 3 bytes or fewer
            return FALSE;
    }
    
    if( strlen(szName) > 8 )
        return FALSE;
    
    //copy file name
    for (count = 0; count < strlen(szName); count++)
    {
        *(fN2 + count) = * (szName + count);
    }
    
    //copy extension
    if(pExt && *pExt )
    {
        for (count = 0; count < strlen (pExt); count++)
        {
            *(fN2 + count + 8) = *(pExt + count);
        }
    }
    
    return TRUE;
}


/******************************************************************************
* Function:        BYTE ValidateChars( char * FileName, BYTE which, BYTE mode)
*
* PreCondition:    None
*
* Input:           fileName   - The name to be validated
*                  which      - PICKFILENAME - Check if chars are valid for FILE names
*                               PICKDIRNAME  - Check if chars are valid for DIR names
*                  mode       - Determines if partial string search is allowed
*
* Output:          TRUE       - Name was validated
*                  FALSE      - File name was not valid
*
* Side Effects:    None
*
* Overview:        Make sure a file name contains valid characters.
*
* Note:            None
*****************************************************************************/
BYTE ValidateChars( char * FileName , BYTE which, BYTE mode)
{
    int StrSz, index;
    
    StrSz = strlen(FileName);
    
    for( index = 0; index < StrSz; index++ )
    {
        if (which == PICKFILENAME)
        {
            switch( isShort( FileName[index], mode) )
            {
                default:
                case 0: // good char
                    break;
    
                case 1: // lower case
                    FileName[index] = toupper( FileName[index] );
                    break;
    
                case 2: // invalid char or space.
                case 3:
                    return FALSE;
                    //FileName[index] = '_';
                    //break;
            }
        }
        else
        {
            if (which == PICKDIRNAME)
            {
                switch (isDIR (FileName[index], mode))
                {
                    case 0:
                        FileName[index] = '_';
                        break;
                    case 1:
                        FileName[index] = toupper(FileName[index]);
                        break;
                    case 2:
                        break;
                }
            }
        }
    }
    return TRUE;
}



/******************************************************************************
* Function:        BYTE isShort (char aChar, BYTE mode)
*
* PreCondition:    None
*
* Input:           aChar   - The char to be checked
*                  mode    - if TRUE, '*' is allowed
*                  
* Output:          0       - The character is acceptable
*                  1       - The char is a lower case letter
*                  2       - Unacceptable char
*                  3       - A space character
*
* Side Effects:    None
*
* Overview:        Checks to see if a char is acceptable in short format
*
* Note:            None
*****************************************************************************/
BYTE isShort ( char aChar, BYTE mode)
{
    if ((aChar >= 'A' && aChar <= 'Z') ||
        (aChar >= '0' && aChar <= '9') ||
        aChar == '$' || aChar == '%' || aChar == '\'' ||
        aChar == '-' || aChar == '_' || aChar == '@' ||
        aChar == '~' || aChar == '`' || aChar == '!' ||
        aChar == '(' || aChar == ')' || aChar == '^' ||
        aChar == '#' || aChar == '&' || aChar == '.')
        return 0;
    else
    {
        if (aChar >= 'a' && aChar <= 'z')
            return 1;
        else
        {
            if (mode)
            {
                if (aChar == '*')
                    return 0;
            }
            if (aChar == ' ')
                return 3;
            else
                return 2;
        }
    }
}


/******************************************************************************
* Function:        BYTE isDIR (BYTE aChar, BYTE mode)
*
* PreCondition:    None
*
* Input:           aChar   - The char to be checked
*                  mode    - if TRUE, '*' is allowed
*                  
* Output:          0       - Replace the char with an underscore
*                  1       - The char is a lower-case letter
*                  2       - The char is acceptable
*
* Side Effects:    None
*
* Overview:        Checks to see if a char is acceptable in a dir name
*
* Note:            None
*****************************************************************************/

BYTE isDIR (BYTE aChar, BYTE mode)
{
    if (mode)
    {
        if (aChar == 0x2A)
            return 2;
    }
    
    if (((aChar < 0x20) && (aChar != 0x05)) || (aChar == 0x22) ||
        (aChar == 0x2A) || (aChar == 0x2B) || (aChar == 0x2C) ||
        (aChar == 0x2E) || (aChar == 0x2F) || (aChar == 0x3A) ||
        (aChar == 0x3B) || (aChar == 0x3C) || (aChar == 0x3D) ||
        (aChar == 0x3E) || (aChar == 0x3F) || (aChar == 0x5B) ||
        (aChar == 0x5C) || (aChar == 0x5D) || (aChar == 0x7C) ||
        (aChar > 0x7F))
    {
        return 0;
    }
    else
    {
        if ((aChar >= 0x61) && (aChar <= 0x7A))
            return 1;
        else
            return 2;
    }
}

/******************************************************************************
* Function:        int FSfseek(FSFILE *stream, long offset, int whence)
*
* PreCondition:    File exists
*
* Input:           stream   - Pointer to file structure
*                  offset   - Offset from origin location
*                  whence   - SEEK_SET - Seek from start of file
*                             SEEK_CUR - Seek from current location
*                             SEEK_END - Seek from end of file (subtract offset)
*
* Output:          0               - Operation successful
*                  -1              - Operation unsuccesful
*
* Side Effects:    None
*
* Overview:        User called function to set the current position of
*                  the stream to a specified value
*
* Note:            None
*****************************************************************************/

int FSfseek(FSFILE *stream, long offset, int whence)
{
    DWORD   numsector, temp;   // lba of first sector of first cluster
    DISK*   dsk;            // pointer to disk structure
    BYTE    test;
    
    dsk = stream->dsk;
    
    switch(whence)
    {
        case SEEK_CUR:
            // Apply the offset to the current position
            offset += stream->seek;
            break;
        case SEEK_END:
            // Apply the offset to the end of the file
            offset = stream->size - offset;
            break;
        case SEEK_SET:
            // automatically there
            default:
            break;
    }

    #ifdef ALLOW_WRITES      
        if (gNeedDataWrite)
            if (flushData())
                return EOF;
    #endif

    // start from the beginning
    temp = stream->cluster;
    stream->ccls = temp;

    temp = stream->size;

    if (offset > temp)
        return (-1);      // past the limits
    else
    {
        // if we are writing we are no longer at the end
        stream->flags.FileWriteEOF = FALSE;
        
        // set the new postion
        stream->seek = offset;
        
        // figure out how many sectors
        numsector = offset / MEDIA_SECTOR_SIZE;
        
        // figure out how many bytes off of the offset
        offset = offset - (numsector * MEDIA_SECTOR_SIZE);
        stream->pos = offset;
        
        // figure out how many clusters
        temp = numsector / dsk->SecPerClus;
        
        // figure out the stranded sectors
        numsector = numsector - (dsk->SecPerClus * temp);
        stream->sec = numsector;
    
        // if we are in the current cluster stay there
        if (temp > 0)
        {
            test = FILEget_next_cluster(stream, temp);
            if (test != CE_GOOD)
            {   
                if (test == CE_FAT_EOF)
                {
                    #ifdef ALLOW_WRITES
                        if (stream->flags.write)
                        {
                            // load the previous cluster
                            stream->ccls = stream->cluster;
                            // Don't perform this operation if there's only one cluster
                            if (temp != 1)
                                test = FILEget_next_cluster(stream, temp - 1);
                            if (FILEallocate_new_cluster(stream, 0) != CE_GOOD)
                                return -1;
                            // sec and pos should already be zero
                        }
                        else
                        {
                    #endif
                    stream->ccls = stream->cluster;
                    test = FILEget_next_cluster(stream, temp - 1);
                    if (test != CE_GOOD)
                        return (-1);
                    stream->pos = MEDIA_SECTOR_SIZE;
                    stream->sec = dsk->SecPerClus - 1;
                    #ifdef ALLOW_WRITES
                        }
                    #endif
                }
                else
                    return (-1);   // past the limits
            }
        }

        // Determine the lba of the selected sector and load
        temp = Cluster2Sector(dsk,stream->ccls);

        // now the extra sectors
        numsector = stream->sec;
        temp += numsector;

        gBufferOwner = NULL;
        gBufferZeroed = FALSE;
        if( !SectorRead(temp, dsk->buffer) )
            return (-1);   // Bad read
        gLastDataSectorRead = temp;
    }
    return (0);
}


// FSfopenpgm, FSremovepgm, and FSrenamepgm will only work on PIC18s
#ifdef USE_PIC18
#ifdef ALLOW_PGMFUNCTIONS


/******************************************************************************
* Function:        int FSrenamepgm(const rom char * fileName, FSFILE * fo)
*
* PreCondition:    None
*
* Input:           fileName   - The new name of the file (ROM)
*                  fo         - The file to rename
*                  
* Output:          int        - Returns 0 if success, -1 otherwise
*
* Side Effects:    None
*
* Overview:        Change the name of a file or directory
*
* Note:            None
*****************************************************************************/

int FSrenamepgm (const rom char * fileName, FSFILE * fo)
{
    char F[13];
    BYTE count;
    
    for (count = 0; count < 13; count++)
    {
        F[count] = *(fileName + count);
    }
    
    return FSrename (F, fo);
}


/******************************************************************************
* Function:        FSFILE * FSfopenpgm(const rom char * fileName, const rom char *mode)
*
* PreCondition:    None
*
* Input:           fileName   - The name of the file to be opened (ROM)
*                  mode       - The mode the file will be opened in (ROM)
*                  
* Output:          FSFILE *     - A pointer to the file object
*
* Side Effects:    None
*
* Overview:        Opens a file if the name and the mode are given in ROM 
*                  format.  Eg. fopenpgm ("FILE.TXT", "w");
*
* Note:            None
*****************************************************************************/

FSFILE * FSfopenpgm(const rom char * fileName, const rom char *mode)
{
    char F[13];
    char M[2];
    BYTE count;
    
    for (count = 0; count < 13; count++)
    {
        F[count] = *(fileName + count);
    }
    for (count = 0; count < 2; count++)
    {
        M[count] = *(mode + count);
    }
    
    return FSfopen(F, M);
}

/******************************************************************************
* Function:        int FSremovepgm (const rom char * fileName)
*
* PreCondition:    None
*
* Input:           fileName   - The name of the file to be deleted (ROM)
*                  
* Output:          int         - 0 is successful, -1 otherwise
*
* Side Effects:    None
*
* Overview:        Removes a file if the name is given in ROM
*                  Eg. FSremovepgm ("FILE.TXT");
*
* Note:            None
*****************************************************************************/
#ifdef ALLOW_WRITES
int FSremovepgm (const rom char * fileName)
{
    char F[13];
    BYTE count;
    
    *fileName;
    for(count = 0; count < sizeof(F); count++)
    {
        _asm TBLRDPOSTINC _endasm
        F[count] = TABLAT;
    }//end for(...)   
    
    return FSremove (F);
}
#endif

/******************************************************************************
* Function:        int FindFirstpgm (const char * fileName, unsigned int attr, SearchRec * rec)
*
* PreCondition:    None
*
* Input:           fileName   - The name of the file to be found (ROM)
*                  attr       - The attributes of the file to be found
*                  rec        - The search record to store the file info in
*                  
* Output:          int         - 0 is successful, -1 otherwise
*
* Side Effects:    None
*
* Overview:        Locates a file on the device
*                  Eg. FindFirstpgm ("FILE.TXT");
*
* Note:            Call FindFirstpgm or FindFirst before calling FindNext
*****************************************************************************/
#ifdef ALLOW_FILESEARCH
int FindFirstpgm (const rom char * fileName, unsigned int attr, SearchRec * rec)
{
    char F[13];
    BYTE count;
    
    *fileName;
    for(count = 0; count < sizeof(F); count++)
    {
        _asm TBLRDPOSTINC _endasm
        F[count] = TABLAT;
    }//end for
    
    return FindFirst (F,attr,rec);
}
#endif
#endif
#endif


/******************************************************************************
* Function:        WORD ReadFAT (DISK *dsk, WORD ccls)
*
* PreCondition:    None
*
* Input:           dsk        - The disk structure
*                  ccls       - The current cluster
*                  
* Output:          WORD       - The next cluster in a file chain
*
* Side Effects:    None
*
* Overview:        Find successive clusters in the FAT
*
* Note:            None
*****************************************************************************/

WORD ReadFAT (DISK *dsk, WORD ccls)
{
    BYTE    q;
    DWORD   p, l;
    WORD    c, d;   
    
    // We only support FAT16 and FAT12 right now   
    if (dsk->type != FAT16 && dsk->type != FAT12)
        return CLUSTER_FAIL;
    
    gBufferZeroed = FALSE;
    
    // Mulby 2 or 1.5 to find cluster pos in FAT
    if (dsk->type == FAT16)
    {
        p = (DWORD)ccls *2;
    }
    if (dsk->type == FAT12)
    {
        p = (DWORD) ccls *3;
        q = p&1;
        p >>= 1;
    }
    
    l = dsk->fat + (p >> 9);
    p &= 0x1ff;
    
    // Check if the appropriate FAT sector is already loaded
    if (gLastFATSectorRead == l)
    {
        if (dsk->type == FAT16)
            c = RAMreadW (gFATBuffer, p);
        else if (dsk->type == FAT12)
        {
            c = RAMread (gFATBuffer, p);
            if (q)
            {
                c >>= 4;   
            }
            // Check if the MSB is across the sector boundry
            p = (p +1) & 0x1FF;
            if (p == 0)
            {
                // Start by writing the sector we just worked on to the card
                // if we need to
                #ifdef ALLOW_WRITES
                    if (gNeedFATWrite)
                        if(WriteFAT (dsk, 0, 0, TRUE))
                            return CLUSTER_FAIL;
                #endif
    
                if (!SectorRead (l+1, gFATBuffer))
                {
                    gLastFATSectorRead = 0xFFFF;
                    return CLUSTER_FAIL;
                }
                else
                {
                    gLastFATSectorRead = l +1;
                }
            }
            d = RAMread (gFATBuffer, p);
            if (q)
            {
                c += (d <<4);
            }
            else
            {
                c += ((d & 0x0F)<<8);
            }
        }
    }
    else
    {
        // If there's a currently open FAT sector,
        // write it back before reading into the buffer
        #ifdef ALLOW_WRITES
            if (gNeedFATWrite)
            {
                if(WriteFAT (dsk, 0, 0, TRUE))
                    return CLUSTER_FAIL;
            }
        #endif
    
        if (!SectorRead (l, gFATBuffer))
        {
            gLastFATSectorRead = 0xFFFF;
            return CLUSTER_FAIL;
        }
        else
        {
            gLastFATSectorRead = l;
            if (dsk->type == FAT16)
                c = RAMreadW (gFATBuffer, p);
            else if (dsk->type == FAT12)
            {
                c = RAMread (gFATBuffer, p);
                if (q)
                {
                    c >>= 4;   
                }
                p = (p +1) & 0x1FF;
                d = RAMread (gFATBuffer, p);
                if (q)
                {
                    c += (d <<4);
                }
                else
                {
                    c += ((d & 0x0F)<<8);
                }
            }
        }
    }
    
    // Normalize it so 0xFFFF is an error
    if (c >= LAST_CLUSTER_FAT16)
        c = LAST_CLUSTER;
    
    return c;
}   // ReadFAT



/******************************************************************************
* Function:        WORD WriteFAT (DISK *dsk, WORD ccls, WORD value, BYTE forceWrite)
*
* PreCondition:    None
*
* Input:           dsk        - The disk structure
*                  ccls       - The current cluster
*                  value      - The value to write in
*                  forceWrite - Force the function to write the current FAT sector
*                  
* Output:          WORD       - 0 if successful, FAIL otherwise
*
* Side Effects:    None
*
* Overview:        Write a value to the FAT
*
* Note:            None
*****************************************************************************/

#ifdef ALLOW_WRITES
WORD WriteFAT (DISK *dsk, WORD ccls, WORD value, BYTE forceWrite)
{
    BYTE    i, q, c;
    DWORD   p, li, l;
    
    if (dsk->type != FAT16 && dsk->type != FAT12)
        return CLUSTER_FAIL;
    
    gBufferZeroed = FALSE;
    
    // The only purpose for calling this function with forceWrite
    // is to write the current FAT sector to the card
    if (forceWrite)
    {
        for (i = 0, li = gLastFATSectorRead; i < dsk->fatcopy; i++, li += dsk->fatsize)
            if (!SectorWrite (gLastFATSectorRead, gFATBuffer, FALSE))
                return CLUSTER_FAIL;
    
        gNeedFATWrite = FALSE;
    
        return 0;
    }
    
    if (dsk->type == FAT16)
    {
        p = (DWORD) ccls *2;
    
        l = dsk->fat + (p >> 9);
    
        p &= 0x1FF;
    }
    else if (dsk->type == FAT12)
    {
        p = (DWORD) ccls * 3;
        q = p & 1;   // Odd or even?
        p >>= 1;
        l = dsk->fat + (p >>9);
        p &= 0x1FF;
    }
    
    if (gLastFATSectorRead != l)
    {
        // If we are loading a new sector then write
        // the current one to the card if we need to
        if (gNeedFATWrite)
        {
            for (i = 0, li = gLastFATSectorRead; i < dsk->fatcopy; i++, li += dsk->fatsize)
                if (!SectorWrite (gLastFATSectorRead, gFATBuffer, FALSE))
                    return CLUSTER_FAIL;
    
            gNeedFATWrite = FALSE;
        }
    
        // Load the new sector
        if (!SectorRead (l, gFATBuffer))
        {
            gLastFATSectorRead = 0xFFFF;
            return CLUSTER_FAIL;
        }
        else
            gLastFATSectorRead = l;
    }
    
    if (dsk->type == FAT16)
    {
        RAMwrite (gFATBuffer, p, value);            //lsB
        RAMwrite (gFATBuffer, p+1, (value >> 8));   //msB
    }
    if (dsk->type == FAT12)
    {
        // Get the current byte from the FAT
        c = RAMread (gFATBuffer, p);
        if (q)
        {
            c = ((value & 0x0F) << 4) | ( c & 0x0F);
        }
        else
        {
            c = (value & 0xFF);
        }
        // Write in those bits
        RAMwrite (gFATBuffer, p, c);
        
        // FAT12 entries can cross sector boundaries
        // Check if we need to load a new sector
        p = (p+1) & 0x1FF;
        if (p == 0)
        {
            // call this function to update the FAT on the card
            if (WriteFAT (dsk, 0,0,TRUE))
                return CLUSTER_FAIL;
            // Load the next sector
            if (!SectorRead (l +1, gFATBuffer))
            {
                gLastFATSectorRead = 0xFFFF;
                return CLUSTER_FAIL;
            }
            else
                gLastFATSectorRead = l + 1;
        }
    
        // Get the second byte of the table entry
        c = RAMread (gFATBuffer, p);
        if (q)
        {
            c = (value >> 4);
        }
        else
        {
            c = ((value >> 8) & 0x0F) | (c & 0xF0);
        }
        RAMwrite (gFATBuffer, p, c);
    }
    
    gNeedFATWrite = TRUE;
    return 0;
}
#endif


#ifdef ALLOW_DIRS

// This string is used by dir functions to hold dir names temporarily
char defaultString [11];


int FSchdir (char * path)
{
    return chdirhelper (0, path, NULL);
}

#ifdef ALLOW_PGMFUNCTIONS
int FSchdirpgm (const rom char * path)
{
    return chdirhelper (1, NULL, path);
}
#endif

/******************************************************************************
* Function:        int FSchdir (char * path)
*
* PreCondition:    None
*
* Input:           path       - The directory to change to
*                  
* Output:          int        - 0 if successful, EOF otherwise
*
* Side Effects:    Modified the current working directory
*
* Overview:        Change directory to the dir specified
*
* Note:            None
*****************************************************************************/

#ifdef ALLOW_PGMFUNCTIONS
int chdirhelper (BYTE mode, char * ramptr, const rom char * romptr)
#else
int chdirhelper (BYTE mode, char * ramptr, char * romptr)
#endif
{
    BYTE        i, j;
    WORD        curent = 1;
    DIRENTRY    entry;
    char *      temppath = ramptr;
    #ifdef ALLOW_PGMFUNCTIONS
        rom char * temppath2 = romptr;
    #endif
    char        tempArray[11];
    FSFILE      tempCWDobj;
    FSFILE      tempfile;
    FILEOBJ     tempCWD = &tempCWDobj;

    FileObjectCopy (tempCWD, cwdptr);
   
   // Check the first char of the path
    #ifdef ALLOW_PGMFUNCTIONS
        if (mode)
            i = *temppath2;
        else
    #endif
            i = *temppath;
    if (i == 0)
        return -1;
    
    while(1)
    {
        switch (i)
        {
            // First case: dot or dotdot entry
            case '.':
                // Move past the dot
                #ifdef ALLOW_PGMFUNCTIONS
                    if (mode)
                    {
                        temppath2++;
                        i = *temppath2;
                    }
                    else
                    {
                #endif
                        temppath++;
                        i = *temppath;
                #ifdef ALLOW_PGMFUNCTIONS
                    } 
                #endif
                // Check if it's a dotdot entry
                if (i == '.')
                {
                    // Increment the path variable
                    #ifdef ALLOW_PGMFUNCTIONS
                        if (mode)
                        {
                            temppath2++;
                            i = *temppath2;
                        }
                        else
                        {
                    #endif
                            temppath++;
                            i = *temppath;
                    #ifdef ALLOW_PGMFUNCTIONS
                        } 
                    #endif               
                    // Check if we're in the root
                    if (tempCWD->dirclus == 0)
                    {
                        // Fails if there's a dotdot chdir from the root
                        return -1;
                    }
                    else
                    {
                        // Cache the dotdot entry
                        tempCWD->dirccls = tempCWD->dirclus;
                        curent = 1;
                        entry = Cache_File_Entry (tempCWD, &curent, TRUE);
                        if (entry == NULL)
                        {
                            return -1;
                        }
                        tempCWD->dirclus = entry->DIR_FstClusLO;
                        tempCWD->dirccls = tempCWD->dirclus;
    
                        // If we changed to root, record the name
                        if (tempCWD->dirclus == 0)
                        {
                            tempCWD->name[0] = '\\';
                            for (j = 1; j < 11; j++)
                            {
                                tempCWD->name[j] = 0x20;
                            }
                        }
                        else
                        {
                            // Otherwise set the name to ..
                            tempCWD->name[0] = '.';
                            tempCWD->name[1] = '.';
                            for (j = 2; j < 11; j++)
                            {
                                tempCWD->name[j] = 0x20;
                            }
                        }
                        // Cache the dot entry
                        curent = 0;
                        if (Cache_File_Entry(tempCWD, &curent, TRUE) == NULL)
                        {
                            return -1;
                        }
                        // Move past the next backslash, if necessary
                        while (i == '\\')
                        {
                            #ifdef ALLOW_PGMFUNCTIONS
                                if (mode)
                                {
                                    temppath2++;
                                    i = *temppath2;
                                }
                                else
                                {
                            #endif
                                    temppath++;
                                    i = *temppath;
                            #ifdef ALLOW_PGMFUNCTIONS
                                } 
                            #endif
                        }
                        // Copy and return, if we're at the end
                        if (i == 0)
                        {
                            FileObjectCopy (cwdptr, tempCWD);
                            return 0;
                        }
                    }
                }
                else
                {
                    // If we ended with a . entry,
                    // just return what we have
                    if (i == 0)
                    {
                        FileObjectCopy (cwdptr, tempCWD);
                        return 0;
                    }
                    else
                    {
                        if (i == '\\')
                        {
                            while (i == '\\')
                            {
                                #ifdef ALLOW_PGMFUNCTIONS
                                    if (mode)
                                    {
                                        temppath2++;
                                        i = *temppath2;
                                    }
                                    else
                                    {
                                #endif
                                        temppath++;
                                        i = *temppath;
                                #ifdef ALLOW_PGMFUNCTIONS
                                    } 
                                #endif
                            }
                            if (i == 0)
                            {
                                FileObjectCopy (cwdptr, tempCWD);
                                return 0;
                            }
                        }
                        else
                        {
                            // Anything else after a dot doesn't make sense
                            return -1;
                        }
                    }
                }
        
                break;
    
            // Second case: the first char is the root backslash
            // We will ONLY switch to this case if the first char
            // of the path is a backslash
            case '\\':
                // Increment pointer to second char
                #ifdef ALLOW_PGMFUNCTIONS
                    if (mode)
                    {
                        temppath2++;
                        i = *temppath2;
                    }
                    else
                    {
                #endif
                        temppath++;
                        i = *temppath;
                #ifdef ALLOW_PGMFUNCTIONS
                    } 
                #endif
                // Can't start the path with multiple backslashes
                if (i == '\\')
                {
                    return -1;
                }
    
                if (i == 0)
                {
                    // The user is changing directory to
                    // the root
                    cwdptr->dirclus = 0;
                    cwdptr->dirccls = 0;
                    cwdptr->name[0] = '\\';
                    for (j = 1; j < 11; j++)
                    {
                        cwdptr->name[j] = 0x20;
                    }
                    return 0;
                }
                else
                {
                    // Our first char is the root dir switch
                    tempCWD->dirclus = 0;
                    tempCWD->dirccls = 0;
                    tempCWD->name[0] = '\\';
                    for (j = 1; j < 11; j++)
                    {
                        tempCWD->name[j] = 0x20;
                    }
                }
                break;

            default:
                // We should be at the beginning of a string of letters/numbers
                j = 0;
                #ifdef ALLOW_PGMFUNCTIONS
                    if (mode)
                    {
                        while ((i != 0) && (i != '\\') && (j < 11))
                        {
                            defaultString[j++] = i;
                            i = *(++temppath2);
                        }
                    }
                    else
                    {
                #endif
                        while ((i != 0) && (i != '\\') && (j < 11))
                        {
                            defaultString[j++] = i;
                            i = *(++temppath);
                        }
                #ifdef ALLOW_PGMFUNCTIONS
                    } 
                #endif   
                // We got a whole 11 chars
                // There could be more- truncate it
                if (j == 11)
                {
                    while ((i != 0) && (i != '\\'))
                    {
                        #ifdef ALLOW_PGMFUNCTIONS
                            if (mode)
                            {
                                i = *(++temppath2);
                            }
                            else
                            {
                        #endif
                                i = *(++temppath);
                        #ifdef ALLOW_PGMFUNCTIONS
                            } 
                        #endif
                    }
                }
                while (j < 11)
                {
                    defaultString[j++] = 0x20;
                }
    
                FormatDirName (defaultString, 0);
    
                for (j = 0; j < 11; j++)
                {
                    tempArray[j] = tempCWD->name[j];
                    tempCWD->name[j] = defaultString[j];
                }
    
                // copy file object over
                FileObjectCopy(&tempfile, tempCWD);
                
                // See if the directory is there
                if(FILEfind (&tempfile, tempCWD, 1, 0) != CE_GOOD)
                {
                    // Couldn't find the DIR
                    return -1;
                }
                else
                {
                    // Found the file
                    // Get the new name
                    for (j = 0; j < 11; j++)
                    {
                        tempCWD->name[j] = tempfile.name[j];
                    }
                    tempCWD->dirclus = tempfile.cluster;
                    tempCWD->dirccls = tempCWD->dirclus;
                }
    
                if (i == 0)
                {
                    // If we're at the end of the string, we're done
                    FileObjectCopy (cwdptr, tempCWD);
                    return 0;
                }
                else
                {
                    while (i == '\\')
                    {
                        // If we get to another backslash, increment past it
                        #ifdef ALLOW_PGMFUNCTIONS
                            if (mode)
                            {
                                temppath2++;
                                i = *temppath2;
                            }
                            else
                            {
                        #endif
                                temppath++;
                                i = *temppath;
                        #ifdef ALLOW_PGMFUNCTIONS
                            } 
                        #endif
                        if (i == 0)
                        {
                            FileObjectCopy (cwdptr, tempCWD);
                            return 0;
                        }
                    }
                }
                break;
        }
    } // loop
}



// This string is used by FSgetcwd to return the cwd name if the path
// passed into the function is NULL
char defaultArray [10];


/******************************************************************************
* Function:        char * FSgetcwd (char * path, int numchars)
*
* PreCondition:    None
*
* Input:           path        - Pointer to the array to return the cwd name in
*                  numchars    - Number of chars in the path
*                  
* Output:          char *      - The cwd name string pointer (path or defaultArray)
*
* Side Effects:    None
*
* Overview:        Gives the user the name of the cwd
*
* Note:            None
*****************************************************************************/


char * FSgetcwd (char * path, int numchars)
{
    // If path is passed in as null, set up a default
    // array with 10 characters
    char        totalchars = (path == NULL) ? 10 : numchars;
    char *      returnPointer;
    char *      bufferEnd;
    FSFILE      tempCWDobj;
    FILEOBJ     tempCWD = &tempCWDobj;
    BYTE        bufferOverflow = FALSE;
    signed char j;
    WORD        curclus, fHandle, tempindex;
    signed int  i, index = 0;
    char        aChar;
    DIRENTRY    entry;
    
    // Set up the return value
    if (path == NULL)
        returnPointer = defaultArray;
    else
        returnPointer = path;
    
    bufferEnd = returnPointer + totalchars - 1;
    
    FileObjectCopy (tempCWD, cwdptr);   
    
    if ((tempCWD->name[0] == '.') &&
        (tempCWD->name[1] == '.'))
    {
        // We last changed directory into a dotdot entry
        // Save the value of the current directory
        curclus = tempCWD->dirclus;
        // Put this dir's dotdot entry into the dirclus
        // Our cwd absolutely is not the root
        fHandle = 1;
        tempCWD->dirccls = tempCWD->dirclus;
        entry = Cache_File_Entry (tempCWD,&fHandle, TRUE);
        if (entry == NULL)
            return NULL;
        tempCWD->dirclus = entry->DIR_FstClusLO;
        tempCWD->dirccls = entry->DIR_FstClusLO;
        // Find the direntry for the entry we were just in
        fHandle = 0;
        entry = Cache_File_Entry (tempCWD, &fHandle, TRUE); 
        if (entry == NULL)
            return NULL;
        while ((entry->DIR_FstClusLO != curclus) || 
            ((entry->DIR_FstClusLO == curclus) && 
            (((unsigned char)entry->DIR_Name[0] == 0xE5) || (entry->DIR_Attr == ATTR_VOLUME) || (entry->DIR_Attr == ATTR_LONG_NAME)))) 
        {
            fHandle++;
            entry = Cache_File_Entry (tempCWD, &fHandle, FALSE); 
            if (entry == NULL)
                return NULL;
        }
        // We've found the entry for the dir we were in      
        for (i = 0; i < 11; i++)
        {
            tempCWD->name[i] = entry->DIR_Name[i];
            cwdptr->name[i] = entry->DIR_Name[i];
        }
        // Reset our temp dir back to that cluster
        tempCWD->dirclus = curclus;
        tempCWD->dirccls = curclus;
        // This will set us at the cwd, but it will actually
        // have the name in the name field this time
    }
    // There's actually some kind of name value in the cwd
    if (tempCWD->name[0] == '\\')
    {
        // Easy, our CWD is the root
        *returnPointer = '\\';
        *(returnPointer + 1) = NULL;
        return returnPointer;
    }
    else
    {
        // Loop until we get back to the root
        while (tempCWD->dirclus != 0)
        {
            // Copy the current name into the buffer backwards
            j = 10;
            while (tempCWD->name[j] == 0x20)
                j--;
            while (j >= 0)
            {
                *(returnPointer + index++) = tempCWD->name[j--];
                // This is a circular buffer
                // Any unnecessary values will be overwritten
                if (index == totalchars)
                {
                    index = 0;
                    bufferOverflow = TRUE;
                }
            }
    
            // Put a backslash delimiter in front of the dir name
            *(returnPointer + index++) = '\\';
            if (index == totalchars)
            {
                index = 0;
                bufferOverflow = TRUE;
            }
    
            // Load the previous entry
            tempCWD->dirccls = tempCWD->dirclus;
            if (GetPreviousEntry (tempCWD))
                return NULL;
    
        }
    }
    
    // Point the index back at the last char in the string
    index--;
    
    i = 0;
    // Swap the chars in the buffer so they are in the right places
    if (bufferOverflow)
    {   
        tempindex = index;
        // Swap the overflowed values in the buffer
        while ((index - i) > 0)
        {
            aChar = *(returnPointer + i);
            *(returnPointer + i) = * (returnPointer + index);
            *(returnPointer + index) = aChar;
            index--;
            i++;
        }
    
        // Point at the non-overflowed values
        i = tempindex + 1;
        index = bufferEnd - returnPointer;
    
        // Swap the non-overflowed values into the right places
        while ((index - i) > 0)
        {
            aChar = *(returnPointer + i);
            *(returnPointer + i) = * (returnPointer + index);
            *(returnPointer + index) = aChar;
            index--;
            i++;
        }
        // All the values should be in the right place now
        // Null-terminate the string
        *(bufferEnd) = 0;
    }
    else
    {
        // There was no overflow, just do one set of swaps
        tempindex = index;
        while ((index - i) > 0)
        {
            aChar = *(returnPointer + i);
            *(returnPointer + i) = * (returnPointer + index);
            *(returnPointer + index) = aChar;
            index--;
            i++;
        }
        *(returnPointer + tempindex + 1) = 0;
    }
    return returnPointer;
}
   
/******************************************************************************
* Function:        void GetPreviousEntry (FSFILE * fo)
*
* PreCondition:    None
*
* Input:           fo          - The file to get the previous entry of
*                  
* Output:          0 if successful, -1 otherwise
*
* Side Effects:    None
*
* Overview:        Gets the file entry information of the dir containing the
*                  dir passed in
*
* Note:            Should not be called by the user
*****************************************************************************/

BYTE GetPreviousEntry (FSFILE * fo)
{
    BYTE        i, test;
    WORD        fHandle = 1;
    WORD        dirclus;
    DIRENTRY    dirptr;
    
    // Load the previous entry
    dirptr = Cache_File_Entry (fo, &fHandle, TRUE);
    if (dirptr == NULL)
        return -1;
    
    if (dirptr->DIR_FstClusLO == 0)
    {
        // The previous directory is the root
        fo->name[0] = '\\';
        for (i = 0; i < 11; i++)
        {
            fo->name[i] = 0x20;
        }
        fo->dirclus = 0;
        fo->dirccls = 0;
    }
    else
    {
        // Get the directory name
        // Save the previous cluster value
        dirclus = dirptr->DIR_FstClusLO;
        fo->dirclus = dirptr->DIR_FstClusLO;
        fo->dirccls = dirptr->DIR_FstClusLO;
        // Load the previous previous cluster
        dirptr = Cache_File_Entry (fo, &fHandle, TRUE);
        if (dirptr == NULL)
            return -1;
        fo->dirclus = dirptr->DIR_FstClusLO;
        fo->dirccls = dirptr->DIR_FstClusLO;
        fHandle = 0;
        dirptr = Cache_File_Entry (fo, &fHandle, TRUE);
        if (dirptr == NULL)
            return -1;
        // Look through it until we get the name 
        // of the previous cluster
        while ((dirptr->DIR_FstClusLO != dirclus) || 
            ((dirptr->DIR_FstClusLO == dirclus) && 
            (((unsigned char)dirptr->DIR_Name[0] == 0xE5) || (dirptr->DIR_Attr == ATTR_VOLUME) || (dirptr->DIR_Attr == ATTR_LONG_NAME)))) 
        {
            // Look through the entries until we get the
            // right one
            dirptr = Cache_File_Entry (fo, &fHandle, FALSE);
            if (dirptr == NULL)
                return -1;
            fHandle++;
        }
    
        test = dirptr->DIR_Name[0];
        // The name should be in the entry now
        // Copy the actual directory location back
        for (i = 0; i < 11; i++)
        {
            fo->name[i] = dirptr->DIR_Name[i];
        }
        fo->dirclus = dirclus;
        fo->dirccls = dirclus;
    }
    return 0;
}



int FSmkdir (char * path)
{
    return mkdirhelper (0, path, NULL);
}

#ifdef ALLOW_PGMFUNCTIONS
int FSmkdirpgm (const rom char * path)
{
    return mkdirhelper (1, NULL, path);
}
#endif


/******************************************************************************
* Function:        int mkdirhelper (char * path)
*
* PreCondition:    None
*
* Input:           path    - The path of the dir to create
*                  
* Output:          int     - 0 if successful, -1 otherwise
*
* Side Effects:    Will create all non-existant directories in the path
*
* Overview:        Create directories
*
* Note:            None
*****************************************************************************/

#ifdef ALLOW_PGMFUNCTIONS
int mkdirhelper (BYTE mode, char * ramptr, const rom char * romptr)
#else
int mkdirhelper (char * path, char * ramptr, char * romptr)
#endif
{
    BYTE        i, j;
    char *      temppath = ramptr;
    #ifdef ALLOW_PGMFUNCTIONS
        rom char *  temppath2 = romptr;
    #endif
    char        tempArray[12];
    FSFILE      tempCWDobj;
    FILEOBJ     tempCWD = &tempCWDobj;
   
    #ifdef USE_PIC18
        char dotdot[] = "..";
    #endif
    
    
    if (WriteProtectState())
    {
        return (-1);
    }
    
    #ifdef ALLOW_PGMFUNCTIONS
        if (mode == 1)
        {
            // Scan for too-long file names
            while (1)
            {
                i = 0;
                while((*temppath2 != 0) && (*temppath2 != '\\'))
                {
                    temppath2++;
                    i++;
                }
                if (i > 8)
                    return -1;
                while (*temppath2 == '\\')
                    temppath2++;
                if (*temppath2 == 0)
                    break;
            }
        }
        else
    #endif
    // Scan for too-long file names
    while (1)
    {
        i = 0;
        while((*temppath != 0) && (*temppath != '\\'))
        {
            temppath++;
            i++;
        }
        if (i > 8)
            return -1;
        while (*temppath == '\\')
            temppath++;
        if (*temppath == 0)
            break;
    }
    
    
    temppath = ramptr;
    #ifdef ALLOW_PGMFUNCTIONS
        temppath2 = romptr;
    #endif
    
    // We're going to be moving the CWD
    // Back up the CWD
    FileObjectCopy (tempCWD, cwdptr);
    
    // get to the target directory
    while (1)
    {
        #ifdef ALLOW_PGMFUNCTIONS
            if (mode == 1)
                i = *temppath2;
            else
        #endif
        i = *temppath;
    
        if (i == '.')
        {
            #ifdef ALLOW_PGMFUNCTIONS
                if (mode == 1)
                {
                    temppath2++;
                    i = *temppath2;
                }
                else
                {
            #endif
                    temppath++;
                    i = *temppath;
            #ifdef ALLOW_PGMFUNCTIONS
                }
            #endif
    
            if (i == '.')
            {
                if (cwdptr->dirclus == 0)
                {
                    // If we try to change to the .. from the
                    // root, operation fails
                    return -1;
                }
                // dotdot entry
                #ifndef USE_PIC18
                    FSchdir ("..");
                #else
                    FSchdir (dotdot);
                #endif
            }
            // Skip past any backslashes
            while (i == '\\')
            {
                #ifdef ALLOW_PGMFUNCTIONS
                    if (mode == 1)
                    {
                        temppath2++;
                        i = *temppath2;
                    }
                    else
                    {
                #endif
                        temppath++;
                        i = *temppath;
                #ifdef ALLOW_PGMFUNCTIONS
                    }
                #endif
            }
            if (i == 0)
            {
                // No point in creating a dot or dotdot entry directly
                FileObjectCopy (cwdptr, tempCWD);
                return -1;
            }
        }
        else
        {
            if (i == '\\')
            {
                // Start at the root
                cwdptr->dirclus = 0;
                cwdptr->dirccls = 0;
                cwdptr->name[0] = '\\';
                for (i = 1; i < 11; i++)
                {
                    cwdptr->name[i] = 0x20;
                }
    
                #ifdef ALLOW_PGMFUNCTIONS
                    if (mode == 1)
                    {
                        temppath2++;
                        i = *temppath2;
                    }
                    else
                    {
                #endif
                        temppath++;
                        i = *temppath; 
                #ifdef ALLOW_PGMFUNCTIONS
                    }
                #endif
                // If we just got two backslashes in a row at the
                // beginning of the path, the function fails
                if (i == '\\')
                {
                    FileObjectCopy (cwdptr, tempCWD);
                    return -1;
                }
                if (i == 0)
                {
                    // We can't make the root dir
                    FileObjectCopy (cwdptr, tempCWD);
                    return -1;
                }
            }
            else
            {
                break;
            }
        }
    }
    
    tempArray[11] = 0;
    while (1)
    {
        while(1)
        {
            #ifdef ALLOW_PGMFUNCTIONS
                if (mode == 1)
                {
                    // Change directories as specified
                    i = *temppath2;
                    j = 0;
                    // Parse the next token
                    while ((i != 0) && (i != '\\') && (j < 11))
                    {
                        tempArray[j++] = i;
                        temppath2++;
                        i = *temppath2;
                    }
                }
                else
                {
            #endif
                    // Change directories as specified
                    i = *temppath;
                    j = 0;
                    // Parse the next token
                    while ((i != 0) && (i != '\\') && (j < 11))
                    {
                        tempArray[j++] = i;
                        temppath++;
                        i = *temppath;
                    }
            #ifdef ALLOW_PGMFUNCTIONS
                }
            #endif
            while (j < 11)
            {
                tempArray[j++] = 0x20;
            }
    
            if (tempArray[0] != '.')
                FormatDirName (tempArray, 0);
            else
            {
                if (tempArray[1] == '.')
                    tempArray[2] = 0;
                else
                    tempArray[1] = 0;
            }
    
            // Try to change to it
            // If you can't we need to create it
            if (FSchdir (tempArray))
            {
                break;
            }
            else
            {
                // We changed into the directory
                while (i == '\\')
                {
                    // Next char is a backslash
                    // Move past it
                    #ifdef ALLOW_PGMFUNCTIONS
                        if (mode == 1)
                        {
                            temppath2++;
                            i = *temppath2;
                        }
                        else
                        {
                    #endif
                            temppath++;
                            i = *temppath;
                    #ifdef ALLOW_PGMFUNCTIONS
                        }
                    #endif
                }
                // If it's the last one, return success
                if (i == 0)
                {
                    FileObjectCopy (cwdptr, tempCWD);
                    return 0;
                }
            }
        }
        // Create a dir here
        if (!CreateDIR (tempArray))
        {
            FileObjectCopy (cwdptr, tempCWD);
            return -1;   
        }
    
        // Try to change to that directory
        if (FSchdir (tempArray))
        {
            FileObjectCopy (cwdptr, tempCWD);
            return -1;
        }
    
        #ifdef ALLOW_PGMFUNCTIONS
            if (mode == 1)
            {
                // Check for another backslash
                while (*temppath2 == '\\')
                {
                    temppath2++;
                    i = *temppath2;
                }
            }
            else
            {
        #endif
                while (*temppath == '\\')
                {
                    temppath++;
                    i = *temppath;
                }
        #ifdef ALLOW_PGMFUNCTIONS
            }
        #endif
    
        // Check to see if we're at the end of the path string
        if (i == 0)
        {
            // We already have one
            FileObjectCopy (cwdptr, tempCWD);
            return 0;
        }   
    }
}


/******************************************************************************
* Function:        int CreateDIR (char * path)
*
* PreCondition:    None
*
* Input:           path      - The name of the dir to create
*                  
* Output:          int       - TRUE   - successful dir creation
*                              FALSE  - dir could not be created
*
* Side Effects:    None
*
* Overview:        Create an actual directory
*
* Note:            Should not be called by the user
*****************************************************************************/

int CreateDIR (char * path)
{
    FSFILE      directoryFile;
    FSFILE *    dirEntryPtr = &directoryFile;
    DIRENTRY    dir;
    WORD        handle = 0, dot, dotdot;
    BYTE        i;
    
    // Copy name into file object
    for (i = 0; i < 11; i++)
    {
        dirEntryPtr->name[i] = *(path + i);
    }
    
    dirEntryPtr->dirclus = cwdptr->dirclus;
    dirEntryPtr->dirccls = cwdptr->dirccls;
    dirEntryPtr->cluster = 0;
    dirEntryPtr->ccls = 0;
    dirEntryPtr->dsk = cwdptr->dsk;
    
    // Create a directory entry
    if( CreateFileEntry(dirEntryPtr, &handle, DIRECTORY) != CE_GOOD)
    {
        return FALSE;
    }
    else
    {            
        if (gNeedFATWrite)
            if(WriteFAT (dirEntryPtr->dsk, 0, 0, TRUE))
                return FALSE;
        // Zero that cluster
        dotdot = dirEntryPtr->dirclus;
        dirEntryPtr->dirccls = dirEntryPtr->dirclus;
        dir = Cache_File_Entry(dirEntryPtr, &handle, TRUE);
        if (dir == NULL)
            return FALSE;
        dot = dir->DIR_FstClusLO;

        if (writeDotEntries (dirEntryPtr->dsk, dot, dotdot))
            return TRUE;
        else
            return FALSE;
    }
}


/******************************************************************************
* Function:        BYTE writeDotEntries (DISK * disk, WORD dotAddress, WORD dotdotAddress)
*
* PreCondition:    None
*
* Input:           disk          - The global disk structure
*                  dotAddress    - The cluster the current dir is in
*                  dotdotAddress - The cluster the previous directory was in
*                  
* Output:          BYTE          - TRUE if successful, FALSE otherwise
*
* Side Effects:    None
*
* Overview:        Create dot and dotdot entries in a subdirectory
*
* Note:            Should not be called by the user
*****************************************************************************/

BYTE writeDotEntries (DISK * disk, WORD dotAddress, WORD dotdotAddress)
{
    WORD        i;
    WORD        size;
    _DIRENTRY   entry;
    DIRENTRY    entryptr = &entry;
    DWORD   sector;

    gBufferOwner = NULL;

    size = sizeof (_DIRENTRY);

    memset(disk->buffer, 0x00, MEDIA_SECTOR_SIZE);

    entry.DIR_Name[0] = '.';

    for (i = 1; i < 11; i++)
    {
        entry.DIR_Name[i] = 0x20;
    }
    entry.DIR_Attr = ATTR_DIRECTORY;
    entry.DIR_NTRes = 0x00;
    entry.DIR_FstClusHI = 0x0000;
    entry.DIR_FstClusLO = dotAddress;
    entry.DIR_FileSize = 0x00;

    // Times need to be the same as the times in the directory entry

    // Set dir date for uncontrolled clock source
    #ifdef INCREMENTTIMESTAMP
        entry.DIR_CrtTimeTenth = 0xB2;
        entry.DIR_CrtTime = 0x7278;
        entry.DIR_CrtDate = 0x32B0;
        entry.DIR_LstAccDate = 0x0000;
        entry.DIR_WrtTime = 0x0000;
        entry.DIR_WrtDate = 0x0000;
    #endif   

    #ifdef USEREALTIMECLOCK
        entry.DIR_CrtTimeTenth = gTimeCrtMS;         // millisecond stamp
        entry.DIR_CrtTime =      gTimeCrtTime;      // time created //
        entry.DIR_CrtDate =      gTimeCrtDate;      // date created (1/1/2004)
        entry.DIR_LstAccDate =   0x0000;         // Last Access date
        entry.DIR_WrtTime =      0x0000;         // last update time
        entry.DIR_WrtDate =      0x0000;         // last update date
    #endif

    #ifdef USERDEFINEDCLOCK
        entry.DIR_CrtTimeTenth = gTimeCrtMS;         // millisecond stamp
        entry.DIR_CrtTime =      gTimeCrtTime;      // time created //
        entry.DIR_CrtDate =      gTimeCrtDate;      // date created (1/1/2004)
        entry.DIR_LstAccDate =   0x0000;         // Last Access date
        entry.DIR_WrtTime =      0x0000;         // last update time
        entry.DIR_WrtDate =      0x0000;         // last update date
    #endif

    for (i = 0; i < size; i++)
    {
        *(disk->buffer + i) = *((char *)entryptr + i);
    }
    entry.DIR_Name[1] = '.';
    entry.DIR_FstClusLO = dotdotAddress;
    for (i = 0; i < size; i++)
    {
        *(disk->buffer + i + size) = *((char *)entryptr + i);
    }

    sector = Cluster2Sector (disk, dotAddress);

    if (SectorWrite(sector, disk->buffer, FALSE) == FALSE)
    {
        return FALSE;
    }
    return TRUE;
}



// This is here to prevent a stack frame error
#ifdef USE_PIC18
    char tempArray[12] = "           ";
#endif



int FSrmdir (char * path, unsigned char rmsubdirs)
{
    return rmdirhelper (0, path, NULL, rmsubdirs);
}

#ifdef ALLOW_PGMFUNCTIONS
int FSrmdirpgm (const rom char * path, unsigned char rmsubdirs)
{
    return rmdirhelper (1, NULL, path, rmsubdirs);
}
#endif

/******************************************************************************
* Function:        int FSrmdir (char * paht, unsigned char rmsubdirs)
*
* PreCondition:    None
*
* Input:           path        - The path of the dir to delete
*                  rmsubdirs   - TRUE if you want to remove all sub-directories
*                                          and files in the directory
*                                FALSE if you want the function to fail if there
*                                          are subdirectories or files in it
*                  
* Output:          None
*
* Side Effects:    None
*
* Overview:        Delete a directory
*
* Note:            None
*****************************************************************************/

#ifdef ALLOW_PGMFUNCTIONS
int rmdirhelper (BYTE mode, char * ramptr, const rom char * romptr, unsigned char rmsubdirs)
#else
int rmdirhelper (BYTE mode, char * ramptr, char * romptr, unsigned char rmsubdirs)
#endif
{
    FSFILE      tempCWDobj, f;
    FILEOBJ     tempCWD = &tempCWDobj;
    FILEOBJ     fo = &f;
    DIRENTRY    entry;
    WORD        handle = 0, handle2;
    WORD        cluster;
    WORD        dotdot;
    BYTE        i;
    BYTE        dirCleared;
    WORD        subDirDepth;
    BYTE        recache = FALSE;
    #ifndef USE_PIC18
        char        tempArray[12] = "           ";
    #endif
    #ifdef USE_PIC18
        char        dotdotname[] = "..";
    #endif

    // Back up the current working directory
    FileObjectCopy (tempCWD, cwdptr);
    
    #ifdef ALLOW_PGMFUNCTIONS
        if (mode)
        {
            if (chdirhelper (1, NULL, romptr))
                return -1;
        }
        else
        {
    #endif
            if (FSchdir (ramptr))
            return -1;
    #ifdef ALLOW_PGMFUNCTIONS
        }
    #endif

    // Make sure we aren't trying to remove the root dir
    entry = Cache_File_Entry (cwdptr, &handle, TRUE);
    if (entry != NULL)
    {
        cluster = entry->DIR_FstClusLO;
        if (cluster == 0 || cluster == tempCWD->dirclus)
        {
            FileObjectCopy (cwdptr, tempCWD);
            return -1;
        }
    }
    else
    {
        FileObjectCopy (cwdptr, tempCWD);
        return -1;
    }

    handle++;
    entry = Cache_File_Entry (cwdptr, &handle, FALSE);
    if (entry != NULL)
    {
        dotdot = entry->DIR_FstClusLO;
    }
    else
    {
        FileObjectCopy (cwdptr, tempCWD);
        return -1;
    }

    handle++;
    entry = Cache_File_Entry (cwdptr, &handle, FALSE);
    if (entry == NULL)
    {
        FileObjectCopy (cwdptr, tempCWD);
        return -1;
    }
    // Don't remove subdirectories and sub-files
    if (!rmsubdirs)
    {
        while (entry->DIR_Name[0] != 0)
        {
            if ((unsigned char)entry->DIR_Name[0] != 0xE5)
            {
                FileObjectCopy (cwdptr, tempCWD);
                return -1;
            }
            handle++;
            entry = Cache_File_Entry (cwdptr, &handle, FALSE);
            if (entry == NULL)
            {
                FileObjectCopy (cwdptr, tempCWD);
                return -1;
            }
        }
    }
    else
    {
        // Do remove subdirectories and sub-files
        dirCleared = FALSE;
        subDirDepth = 0;
        fo->dsk = &gDiskData;

        while (!dirCleared)
        {
            if (entry->DIR_Name[0] != 0)
            {
                if (((unsigned char)entry->DIR_Name[0] != 0xE5) && (entry->DIR_Attr != ATTR_VOLUME) && (entry->DIR_Attr != ATTR_LONG_NAME))
                {
                    if ((entry->DIR_Attr & ATTR_DIRECTORY) == ATTR_DIRECTORY)
                    {
                        // We have a directory
                        subDirDepth++;
                        memset (tempArray, 0x00, 12);
                        for (i = 0; i < 11; i++)
                        {
                            tempArray[i] = entry->DIR_Name[i];
                        }
                        // Change to the subdirectory
                        if (FSchdir (tempArray))
                        {
                            FileObjectCopy (cwdptr, tempCWD);
                            return -1;
                        }
                        else
                        {
                            // Make sure we're not trying to delete the CWD
                            if (cwdptr->dirclus == tempCWD->dirclus)
                            {
                                FileObjectCopy (cwdptr, tempCWD);
                                return -1;
                            }
                        }
                        handle = 2;
                        recache = TRUE;
                    }
                    else
                    {
                        memset (tempArray, 0x00, 12);
                        for (i = 0; i < 11; i++)
                        {
                            fo->name[i] = entry->DIR_Name[i];
                        }
                        
                        fo->entry = handle;
                        fo->dirclus = cwdptr->dirclus;
                        fo->dirccls = cwdptr->dirccls;
                        fo->cluster = 0;
                        fo->ccls    = 0;

                        if (FILEerase(fo, &handle, TRUE))
                        {
                            FileObjectCopy (cwdptr, tempCWD);
                            return -1;
                        }
                        else
                        {
                            handle++;
                        }
                    } // Check to see if it's a DIR entry
                }// Check non-dir entry to see if its a valid file
                else
                {
                    handle++;
                }
                if (recache)
                {
                    recache = FALSE;
                    cwdptr->dirccls = cwdptr->dirclus;
                    entry = Cache_File_Entry (cwdptr, &handle, TRUE);
                }
                else
                {
                    entry = Cache_File_Entry (cwdptr, &handle, FALSE);
                }
                if (entry == NULL)
                {
                    FileObjectCopy (cwdptr, tempCWD);
                    return -1;
                }
            }
            else
            {
                // We have reached the end of the directory
                if (subDirDepth != 0)
                {
                    handle2 = 0;

                    cwdptr->dirccls = cwdptr->dirclus;
                    entry = Cache_File_Entry (cwdptr, &handle2, TRUE);
                    if (entry == NULL)
                    {
                        FileObjectCopy (cwdptr, tempCWD);
                        return -1;
                    }

                    handle2 = entry->DIR_FstClusLO;
                    #ifndef USE_PIC18
                        if (FSchdir (".."))
                    #else
                        if (FSchdir (dotdotname))
                    #endif               
                    {
                        FileObjectCopy (cwdptr, tempCWD);
                        return -1;
                    }
                    // Return to our previous position in this directory
                    handle = 2;
                    cwdptr->dirccls = cwdptr->dirclus;
                    entry = Cache_File_Entry (cwdptr, &handle, TRUE);
                    if (entry == NULL)
                    {
                        FileObjectCopy (cwdptr, tempCWD);
                        return -1;
                    }

                    while ((entry->DIR_FstClusLO != handle2) || 
                        ((entry->DIR_FstClusLO == handle2) && 
                        (((unsigned char)entry->DIR_Name[0] == 0xE5) || (entry->DIR_Attr == ATTR_VOLUME)))) 
                    {
                        handle++;
                        entry = Cache_File_Entry (cwdptr, &handle, FALSE);
                        if (entry == NULL)
                        {
                            FileObjectCopy (cwdptr, tempCWD);
                            return -1;
                        }
                    }
                    // Erase the directory that we just cleared the subdirectories out of
                    memset (tempArray, 0x00, 12);
                    for (i = 0; i < 11; i++)
                    {
                        tempArray[i] = entry->DIR_Name[i];
                    }
                    if (eraseDir (tempArray))
                    {
                        FileObjectCopy (cwdptr, tempCWD);
                        return -1;
                    }
                    else
                    {
                    handle++;
                    cwdptr->dirccls = cwdptr->dirclus;
                    entry = Cache_File_Entry (cwdptr, &handle, TRUE);
                    if (entry == NULL)
                    {
                        FileObjectCopy (cwdptr, tempCWD);
                        return -1;
                    }
                }

                // Decrease the subdirectory depth
                subDirDepth--;
                }
                else
                {
                    dirCleared = TRUE;
                } // Check subdirectory depth
            } // Check until we get an empty entry
        } // Loop until the whole dir is cleared
    }

    // Cache the current directory name
    // tempArray is used so we don't disturb the
    // global getcwd buffer
    if (FSgetcwd (tempArray, 12) == NULL)
    {
        FileObjectCopy (cwdptr, tempCWD);
        return -1;
    }

    memset(tempArray, 0x00, 12);

    for (i = 0; i < 11; i++)
    {
        tempArray[i] = cwdptr->name[i];
    }

    // If we're here, this directory is empty
    #ifndef USE_PIC18
        if (FSchdir (".."))
    #else
        if (FSchdir (dotdotname))
    #endif
    {
        FileObjectCopy (cwdptr, tempCWD);
        return -1;
    }

    if (eraseDir (tempArray))
    {
        FileObjectCopy (cwdptr, tempCWD);
        return -1;
    }
    else
    {
        FileObjectCopy (cwdptr, tempCWD);
        return 0;
    }   
}

   

/******************************************************************************
* Function:        int eraseDir (char * path)
*
* PreCondition:    None
*
* Input:           path      - The name of the directory to delete
*                  
* Output:          int       - 0 if successful, -1 otherwise
*
* Side Effects:    None
*
* Overview:        Erases a directory
*
* Note:            Should not be called by the user
*****************************************************************************/

int eraseDir (char * path)
{
    FSFILE      gblFileTemp, tempCWD;
    CETYPE      result;
    BYTE        i;   
    
    if (WriteProtectState())
    {
        return (-1);
    }
    
    // preserve CWD
    FileObjectCopy(&tempCWD, cwdptr);
    
    for (i = 0; i <11; i++)
    {
        cwdptr->name[i] = *(path + i);
    }
    
    // copy file object over
    FileObjectCopy(&gblFileTemp, cwdptr);
    
    // See if the file is found
    result = FILEfind (cwdptr, &gblFileTemp, 1, 0);
    
    if (result != CE_GOOD)
    {
        FileObjectCopy(cwdptr, &tempCWD);
        return -1;
    }
    result = FILEerase(cwdptr, &cwdptr->entry, TRUE);
    if( result == CE_GOOD )
    {
        FileObjectCopy(cwdptr, &tempCWD);
        return 0;
    }
    else
    {
        FileObjectCopy(cwdptr, &tempCWD);
        return -1;
    }
}



/******************************************************************************
* Function:        void FormatDirName (char * string, BYTE mode)
*
* PreCondition:    None
*
* Input:           string    - The name to be formatted
*                  mode      - TRUE if partial string search character (*)
*                                       is allowed
*                              FALSE otherwise
*                  
* Output:          None
*
* Side Effects:    None
*
* Overview:        Makes sure the dir name is in the correct format
*
* Note:            Should not be called by the user
*****************************************************************************/

void FormatDirName (char * string, BYTE mode)
{
    char    tempString [12];
    BYTE    i;
    
    for (i = 0; i < 8; i++)
    {
        tempString[i] = *(string + i);
    }
    
    tempString[8] = 0x20;
    tempString[9] = 0x20;
    tempString[10] = 0x20;
    tempString[11] = 0;
    
    // Forbidden
    if (tempString[0] == 0x20)
    {
        tempString[0] = '_';
    }
    
    ValidateChars (tempString, PICKDIRNAME, mode);
    
    for (i = 0; i < 11; i++)
    {
        *(string + i) = tempString[i];
    }
    
    return;
}

#endif


#ifdef ALLOW_FILESEARCH


/******************************************************************************
* Function:        int FindFirst (const char * fileName, unsigned int attr, SearchRec * rec)
*
* PreCondition:    None
*
* Input:           fileName      - The name to search for
*                                    Formats:
*                                        *.*          - Find any file or directory
*                                        FILENAME.*   - Find a file or directory with that name 
*                                                            and any extension
*                                        *.EXT        - Find a file with that extension and any name
*                                        FILENAME.EXT - Find a file with that name and extension
*                                        DIR_NAME     - Find a directory with that name
*                                        *            - Find any directory
*                                        FILE*.E*     - Partial string search. Find a file with name
*                                                           starting with "FILE" and extension 
*                                                           starting with "E"
*
*                  attr          - The attributes that a found file may have
*                                      ATTR_READ_ONLY  - File may be read only
*                                      ATTR_HIDDEN     - File may be a hidden file
*                                      ATTR_SYSTEM     - File may be a system file
*                                      ATTR_VOLUME     - Entry may be a volume label
*                                      ATTR_DIRECTORY  - File may be a directory
*                                      ATTR_ARCHIVE    - File may have archive attribute     
*                                      ATTR_MASK       - All attributes
*                  rec            - pointer to a structure to put the file information in
*                  
* Output:          int            - Returns 0 if successful, -1 otherwise
*
* Side Effects:    Search criteria from previous FindFirst call on passed SearchRec object will be lost
*
* Overview:        Finds a file based on parameters passed in by the user
*
* Note:            Call FindFirst or FindFirstpgm before calling FindNext
*****************************************************************************/

int FindFirst (const char * fileName, unsigned int attr, SearchRec * rec)
{
    FSFILE      f;
    FSFILE      gblFileTemp;
    FILEOBJ     fo = &f;
    CETYPE      result;
    WORD        fHandle;
    BYTE        i, j;
    DIRENTRY    dir;
    
    if( !FormatFileName(fileName, fo->name, 1) )
        return -1;
    
    rec->initialized = FALSE;
    
    for (i = 0; (i < 12) && (fileName[i] != 0); i++)
    {
        rec->searchname[i] = fileName[i];
    }
    rec->searchname[i] = 0;
    rec->searchattr = attr;
    #ifdef ALLOW_DIRS
        rec->cwdclus = cwdptr->dirclus;
    #else
        rec->cwdclus = 0;
    #endif
    
    fo->dsk = &gDiskData;
    fo->cluster = 0;
    fo->ccls    = 0;
    fo->entry = 0;
    fo->attributes = attr;
    
    #ifndef ALLOW_DIRS
        // start at the root directory
        fo->dirclus    = 0;
        fo->dirccls    = 0;
    #else
        fo->dirclus = cwdptr->dirclus;
        fo->dirccls = cwdptr->dirccls;
    #endif
    
    // copy file object over
    FileObjectCopy(&gblFileTemp, fo);
    
    // See if the file is found
    result = FILEfind (fo, &gblFileTemp, 1, 1);
    
    if (result != CE_GOOD)
        return -1;
    else
    {
        fHandle = fo->entry;
        result = FILEopen (fo, &fHandle, 'r');
    }
    if (result == CE_GOOD)
    {
        // Copy as much name as there is
        if (fo->attributes != ATTR_VOLUME && fo->attributes != ATTR_DIRECTORY)
        {
            for (i = 0, j = 0; (j < 8) && (fo->name[j] != 0x20); i++, j++)
            {
                rec->filename[i] = fo->name[j];
            }
            // Add the radix if its not a dir
            if ((fo->name[8] != ' ') || (fo->name[9] != ' ') || (fo->name[10] != ' '))
                rec->filename[i++] = '.';
            // Move to the extension, even if there are more space chars
            for (j = 8; (j < 11) && (fo->name[j] != 0x20); i++, j++)
            {
                rec->filename[i] = fo->name[j];
            }
            // Null terminate it
            rec->filename[i] = 0;
        }
        else
        {
            for (i = 0; i < 11; i++)
            {
                rec->filename[i] = fo->name[i];
            }
            rec->filename[i] = 0;
            i--;
            while (rec->filename[i] == 0x20)
                rec->filename[i--] = 0;
        }
    
        rec->attributes = fo->attributes;
        rec->filesize = fo->size;
        if ((fo->attributes & ATTR_DIRECTORY) == 0)
            rec->timestamp = (DWORD)((DWORD)fo->date << 16) + fo->time;
        else
        {
            dir = Cache_File_Entry (fo, &fo->entry, FALSE);
            rec->timestamp = (DWORD)((DWORD)dir->DIR_CrtDate << 16) + dir->DIR_CrtTime;
        }
        rec->entry = fo->entry;
        rec->initialized = TRUE;
        return 0;
    }
    else
        return -1;
}


/******************************************************************************
* Function:        int FindNext (SearchRec * rec)
*
* PreCondition:    None
*
* Input:           rec - The structure to store the file information in
*                  
* Output:          int - 0 if file found, -1 otherwise
*
* Side Effects:    None
*
* Overview:        Finds another file with the criteria specified in your last
*                  call of FindFirst or FindFirstpgm
*
* Note:            Call FindFirst or FindFirstpgm before calling this function
*****************************************************************************/

int FindNext (SearchRec * rec)
{   
    FSFILE      f;
    FSFILE      gblFileTemp;
    FILEOBJ     fo = &f;
    CETYPE      result;
    BYTE        i, j;
    DIRENTRY    dir;
    
    // Make sure we called FindFirst on this object
    if (rec->initialized == FALSE)
        return -1;
    
    // Make we called FindFirst in the cwd
    #ifdef ALLOW_DIRS
        if (rec->cwdclus != cwdptr->dirclus)
            return -1;
    #endif
    
    if( !FormatFileName(rec->searchname, fo->name, 1) )
        return -1;
    
    fo->dsk = &gDiskData;
    fo->cluster = 0;
    fo->ccls    = 0;
    fo->entry = rec->entry + 1;
    fo->attributes = rec->searchattr;
    
    #ifndef ALLOW_DIRS
        // start at the root directory
        fo->dirclus    = 0;
        fo->dirccls    = 0;
    #else
        fo->dirclus = cwdptr->dirclus;
        fo->dirccls = cwdptr->dirccls;
    #endif
    
    // copy file object over
    FileObjectCopy(&gblFileTemp, fo);
    
    // See if the file is found
    result = FILEfind (fo, &gblFileTemp, 1, 1);
    
    if (result != CE_GOOD)
        return -1;
    else
    {
        if (fo->attributes != ATTR_VOLUME && fo->attributes != ATTR_DIRECTORY)
        {
            for (i = 0, j = 0; (j < 8) && (fo->name[j] != 0x20); i++, j++)
            {
                rec->filename[i] = fo->name[j];
            }
            // Add the radix if its not a dir
            if ((fo->name[8] != ' ') || (fo->name[9] != ' ') || (fo->name[10] != ' '))
                rec->filename[i++] = '.';
            // Move to the extension, even if there are more space chars
            for (j = 8; (j < 11) && (fo->name[j] != 0x20); i++, j++)
            {
                rec->filename[i] = fo->name[j];
            }
            // Null terminate it
            rec->filename[i] = 0;
        }
        else
        {
            for (i = 0; i < 11; i++)
            {
                rec->filename[i] = fo->name[i];
            }
            rec->filename[i] = 0;
            i--;
            while (rec->filename[i] == 0x20)
                rec->filename[i--] = 0;
        }
    
        rec->attributes = fo->attributes;
        rec->filesize = fo->size;
        if ((fo->attributes & ATTR_DIRECTORY) == 0)
            rec->timestamp = (DWORD)((DWORD)fo->date << 16) + fo->time;
        else
        {
            dir = Cache_File_Entry (fo, &fo->entry, FALSE);
            rec->timestamp = (DWORD)((DWORD)dir->DIR_CrtDate << 16) + dir->DIR_CrtTime;
        }
        rec->entry = fo->entry;
        return 0;
    }
}


#endif



#ifdef ALLOW_FSFPRINTF

#ifdef USE_PIC18
int FSfprintf (FSFILE *fptr, const rom char *fmt, ...)
#else
int FSfprintf (FSFILE *fptr, const char * fmt, ...)
#endif
{
    va_list ap;
    int n;
    
    va_start (ap, fmt);
    n = FSvfprintf (fptr, fmt, ap);
    va_end (ap);
    return n;
}

#ifdef USE_PIC18
int FSvfprintf (auto FSFILE *handle, auto const rom char *f, auto va_list ap)
#else
int FSvfprintf (FSFILE *handle, const char * f, va_list ap)
#endif
{
    unsigned char   c;
    int             count = 0;

    for (c = *f; c; c = *++f)
    {
        if (c == '%')
        {
            unsigned char       flags = 0;
            unsigned char       width = 0;
            unsigned char       precision = 0;
            unsigned char       have_precision = 0;
            unsigned char       size = 0;
            #ifndef USE_PIC18
                unsigned char       size2 = 0;
            #endif
            unsigned char       space_cnt;
            unsigned char       cval;
            #ifdef USE_PIC18
                unsigned long       larg;
                far rom char *      romstring;
            #else
                unsigned long long  larg;
            #endif
            char *              ramstring;
            int                 n;
            c = *++f;


            while (c == '-' || c == '+' || c == ' ' || c == '#'
                || c == '0')
            {
                switch (c)
                {
                    case '-':
                        flags |= _FLAG_MINUS;
                        break;
                    case '+':
                        flags |= _FLAG_PLUS;
                      break;
                    case ' ':
                        flags |= _FLAG_SPACE;
                        break;
                    case '#':
                        flags |= _FLAG_OCTO;
                        break;
                    case '0':
                        flags |= _FLAG_ZERO;
                        break;
                }
                c = *++f;
            }
            /* the optional width field is next */
            if (c == '*')
            {
                n = va_arg (ap, int);
                if (n < 0)
                {
                    flags |= _FLAG_MINUS;
                    width = -n;
                }
                else
                    width = n;
                c = *++f;
            }
            else
            {
                cval = 0;
                while ((unsigned char) isdigit (c))
                {
                    cval = cval * 10 + c - '0';
                    c = *++f;
                }
                width = cval;
            }

            /* if '-' is specified, '0' is ignored */
            if (flags & _FLAG_MINUS)
                flags &= ~_FLAG_ZERO;

            /* the optional precision field is next */
            if (c == '.')
            {
                c = *++f;
                if (c == '*')
                {
                    n = va_arg (ap, int);
                    if (n >= 0)
                    {
                        precision = n;
                        have_precision = 1;
                    }
                    c = *++f;
                }
                else
                {
                    cval = 0;
                    while ((unsigned char) isdigit (c))
                    {
                        cval = cval * 10 + c - '0';
                        c = *++f;
                    }
                    precision = cval;
                    have_precision = 1;
                }
            }

            /* the optional 'h' specifier. since int and short int are
            the same size for MPLAB C18, this is a NOP for us. */
            if (c == 'h')
            {
                c = *++f;
                /* if 'c' is another 'h' character, this is an 'hh'
                specifier and the size is 8 bits */
                if (c == 'h')
                {
                    size = _FMT_BYTE;
                    c = *++f;
                }
            }
            else if (c == 't' || c == 'z')
                c = *++f;
            #ifdef USE_PIC18
                else if (c == 'H' || c == 'T' || c == 'Z')
                {
                    size = _FMT_SHRTLONG;
                    c = *++f;
                }
                else if (c == 'l' || c == 'j')
            #else
                else if (c == 'q' || c == 'j')
                {
                    size = _FMT_LONGLONG;
                    c = *++f;
                }
                else if (c == 'l')
            #endif
            {
            size = _FMT_LONG;
            c = *++f;
            }


            switch (c)
            {
                case '\0':
                    /* this is undefined behaviour. we have a trailing '%' character
                    in the string, perhaps with some flags, width, precision
                    stuff as well, but no format specifier. We'll, arbitrarily,
                    back up a character so that the loop will terminate 
                    properly when it loops back and we'll output a '%'
                    character. */
                    --f;
                    /* fallthrough */
                case '%':
                    if (FSputc ('%', handle) == EOF)
                        return EOF;
                    ++count;
                    break;
                case 'c':
                    space_cnt = 0;
                    if (width > 1)
                    {
                        space_cnt = width - 1;
                        count += space_cnt;
                    }
                    if (space_cnt && !(flags & _FLAG_MINUS))
                    {
                        if (str_put_n_chars (handle, space_cnt, ' '))
                            return EOF;
                        space_cnt = 0;
                    }
                    c = va_arg (ap, int);
                    if (FSputc (c, handle) == EOF)
                        return EOF;
                    ++count;
                    if (str_put_n_chars (handle, space_cnt, ' '))
                        return EOF;
                    break;
                case 'S':
                    #ifdef USE_PIC18
                        if (size == _FMT_SHRTLONG)
                            romstring = va_arg (ap, rom far char *);
                        else
                            romstring = (far rom char*)va_arg (ap, rom near char *);
                        n = strlenpgm (romstring);
                        /* Normalize the width based on the length of the actual 
                        string and the precision. */
                        if (have_precision && precision < (unsigned char) n)
                            n = precision;
                        if (width < (unsigned char) n)
                            width = n;
                        space_cnt = width - (unsigned char) n;
                        count += space_cnt;
                        /* we've already calculated the space count that the width
                        will require. now we want the width field to have the
                        number of character to display from the string itself,
                        limited by the length of the actual string and the
                        specified precision. */
                        if (have_precision && precision < width)
                            width = precision;
                        /* if right justified, we print the spaces before the
                        string */
                        if (!(flags & _FLAG_MINUS))
                        {
                            if (str_put_n_chars (handle, space_cnt, ' '))
                                return EOF;
                            space_cnt = 0;
                        }
                        cval = 0;
                        for (c = *romstring; c && cval < width; c = *++romstring)
                        {
                            if (FSputc (c, handle) == EOF)
                                return EOF;
                            ++count;
                            ++cval;
                        }
                        /* If there are spaces left, it's left justified. 
                        Either way, calling the function unconditionally 
                        is smaller code. */
                        if (str_put_n_chars (handle, space_cnt, ' '))
                            return EOF;
                        break;
                    #endif
                case 's':
                    ramstring = va_arg (ap, char *);
                    n = strlen (ramstring);
                    /* Normalize the width based on the length of the actual 
                    string and the precision. */
                    if (have_precision && precision < (unsigned char) n)
                        n = precision;
                    if (width < (unsigned char) n)
                        width = n;
                    space_cnt = width - (unsigned char) n;
                    count += space_cnt;
                    /* we've already calculated the space count that the width
                    will require. now we want the width field to have the
                    number of character to display from the string itself,
                    limited by the length of the actual string and the
                    specified precision. */
                    if (have_precision && precision < width)
                        width = precision;
                    /* if right justified, we print the spaces before the
                    string */
                    if (!(flags & _FLAG_MINUS))
                    {
                        if (str_put_n_chars (handle, space_cnt, ' '))
                            return EOF;
                        space_cnt = 0;
                    }
                    cval = 0;
                    for (c = *ramstring; c && cval < width; c = *++ramstring)
                    {
                        if (FSputc (c, handle) == EOF)
                            return EOF;
                        ++count;
                        ++cval;
                    }
                    /* If there are spaces left, it's left justified. 
                    Either way, calling the function unconditionally 
                    is smaller code. */
                    if (str_put_n_chars (handle, space_cnt, ' '))
                        return EOF;
                    break;
                case 'd':
                case 'i':
                    flags |= _FLAG_SIGNED;
                    /* fall through */
                case 'o':
                case 'u':
                case 'x':
                case 'X':
                case 'b':
                case 'B':
                    /* This is a bit of a trick. The 'l' and 'hh' size
                    specifiers are valid only for the integer conversions,
                    not the 'p' or 'P' conversions, and are ignored for the
                    latter. By jumping over the additional size specifier
                    checks here we get the best code size since we can
                    limit the size checks in the remaining code. */
                    if (size == _FMT_LONG)
                    {
                        if (flags & _FLAG_SIGNED)
                            larg = va_arg (ap, long int);
                        else
                            larg = va_arg (ap, unsigned long int);
                        goto _do_integer_conversion;
                    }
                    else if (size == _FMT_BYTE)
                    {
                        if (flags & _FLAG_SIGNED)
                            larg = (signed char) va_arg (ap, int);
                        else
                            larg = (unsigned char) va_arg (ap, unsigned int);
                        goto _do_integer_conversion;
                    }
                    #ifndef USE_PIC18
                        else if (size == _FMT_LONGLONG)
                        {
                            if (flags & _FLAG_SIGNED)
                                larg = (signed long long)va_arg (ap, long long);
                            else
                                larg = (unsigned long long) va_arg (ap, unsigned long long);
                            goto _do_integer_conversion;
                        }
                    #endif
                    /* fall trough */
                case 'p':
                case 'P':
                    #ifdef USE_PIC18
                        if (size == _FMT_SHRTLONG)
                        {
                            if (flags & _FLAG_SIGNED)
                                larg = va_arg (ap, short long int);
                            else
                                larg = va_arg (ap, unsigned short long int);
                        }
                        else 
                    #endif
                        if (flags & _FLAG_SIGNED)
                            larg = va_arg (ap, int);
                        else
                            larg = va_arg (ap, unsigned int);
_do_integer_conversion:
                    /* default precision is 1 */
                    if (!have_precision)
                        precision = 1;
                    {
                        unsigned char digit_cnt = 0;
                        unsigned char prefix_cnt = 0;
                        unsigned char sign_char;
                        /* A 32 bit number will require at most 32 digits in the
                        string representation (binary format). */
                        #ifdef USE_PIC18
                            char buf[33];
                            /* Start storing digits least-significant first */
                            char *q = &buf[31];
                            /* null terminate the string */
                            buf[32] = '\0';
                        #else
                            char buf[65];
                            char *q = &buf[63];
                            buf[64] = '\0';
                        #endif
                        space_cnt = 0;
                        size = 10;

                        switch (c)
                        {
                            case 'b':
                            case 'B':
                                size = 2;
                                #ifndef USE_PIC18
                                    size2 = 1;
                                #endif
                                break;
                            case 'o':
                                size = 8;
                                #ifndef USE_PIC18
                                    size2 = 3;
                                #endif
                                break;
                            case 'p':
                            case 'P':
                                /* from here on out, treat 'p' conversions just
                                like 'x' conversions. */
                                c += 'x' - 'p';
                                /* fall through */
                            case 'x':
                            case 'X':
                                size = 16;
                                #ifndef USE_PIC18
                                    size2 = 4;
                                #endif
                                break;
                        } // switch (c)

                        /* if it's an unsigned conversion, we should ignore the
                        ' ' and '+' flags */
                        if (!(flags & _FLAG_SIGNED))
                            flags &= ~(_FLAG_PLUS | _FLAG_SPACE);

                        /* if it's a negative value, we need to negate the
                        unsigned version before we convert to text. Using
                        unsigned for this allows us to (ab)use the 2's
                        complement system to avoid overflow and be able to
                        adequately handle LONG_MIN.
                        
                        We'll figure out what sign character to print, if
                        any, here as well. */
                        #ifdef USE_PIC18
                            if (flags & _FLAG_SIGNED && ((long) larg < 0))
                            {
                                larg = -(long) larg;
                        #else
                            if (flags & _FLAG_SIGNED && ((long long) larg < 0))
                            {
                                larg = -(long long) larg;
                        #endif
                            sign_char = '-';
                            ++digit_cnt;
                        }
                        else if (flags & _FLAG_PLUS)
                        {
                            sign_char = '+';
                            ++digit_cnt;
                        }
                        else if (flags & _FLAG_SPACE)
                        {
                            sign_char = ' ';
                            ++digit_cnt;
                        }
                        else
                            sign_char = '\0';
                        /* get the digits for the actual number. If the
                        precision is zero and the value is zero, the result
                        is no characters. */
                        if (precision || larg)
                        {
                            do
                            {
                                #ifdef USE_PIC18
                                    cval = s_digits[larg % size];
                                    if (c == 'X' && cval >= 'a')
                                        cval -= 'a' - 'A';
                                    larg /= size;
                                #else
                                    // larg is congruent mod size2 to its lower 16 bits
                                    // for size2 = 2^n, 0 <= n <= 4
                                    if (size2 != 0)
                                        cval = s_digits[(unsigned int) larg % size];
                                    else
                                        cval = s_digits[larg % size];
                                    if (c == 'X' && cval >= 'a')
                                        cval -= 'a' - 'A';
                                    if (size2 != 0)
                                        larg = larg >> size2;
                                    else
                                        larg /= size;
                                #endif
                                *q-- = cval;
                                ++digit_cnt;
                            } while (larg);
                            /* if the '#' flag was specified and we're dealing
                            with an 'o', 'b', 'B', 'x', or 'X' conversion,
                            we need a bit more. */
                            if (flags & _FLAG_OCTO)
                            {
                                if (c == 'o')
                                {
                                    /* per the standard, for octal, the '#' flag
                                    makes the precision be at least one more
                                    than the number of digits in the number */
                                    if (precision <= digit_cnt)
                                        precision = digit_cnt + 1;
                                }
                                else if (c == 'x' || c == 'X' || c == 'b' || c == 'B')
                                    prefix_cnt = 2;
                            }
                        }
                        else
                            digit_cnt = 0;

                        /* The leading zero count depends on whether the '0'
                        flag was specified or not. If it was not, then the
                        count is the difference between the specified
                        precision and the number of digits (including the
                        sign character, if any) to be printed; otherwise,
                        it's as if the precision were equal to the max of
                        the specified precision and the field width. If a
                        precision was specified, the '0' flag is ignored,
                        however. */
                        if ((flags & _FLAG_ZERO) && (width > precision)
                            && !have_precision)
                            precision = width;
                        /* for the rest of the processing, precision contains
                        the leading zero count for the conversion. */
                        if (precision > digit_cnt)
                            precision -= digit_cnt;
                        else
                            precision = 0;
                        /* the space count is the difference between the field
                        width and the digit count plus the leading zero
                        count. If the width is less than the digit count
                        plus the leading zero count, the space count is
                        zero. */
                        if (width > precision + digit_cnt + prefix_cnt)
                            space_cnt =   width - precision - digit_cnt - prefix_cnt;

                        /* for output, we check the justification, if it's
                        right justified and the space count is positive, we
                        emit the space characters first. */
                        if (!(flags & _FLAG_MINUS) && space_cnt)
                        {
                            if (str_put_n_chars (handle, space_cnt, ' '))
                                return EOF;
                            count += space_cnt;
                            space_cnt = 0;
                        }
                        /* if we have a sign character to print, that comes
                        next */
                        if (sign_char)
                            if (FSputc (sign_char, handle) == EOF)
                                return EOF;
                        /* if we have a prefix (0b, 0B, 0x or 0X), that's next */
                        if (prefix_cnt)
                        {
                            if (FSputc ('0', handle) == EOF)
                                return EOF;
                            if (FSputc (c, handle) == EOF)
                                return EOF;
                        }
                        /* if we have leading zeros, they follow. the prefix, if any
                        is included in the number of digits when determining how
                        many leading zeroes are needed. */
                        if (precision > prefix_cnt)
                            precision -= prefix_cnt;
                        if (str_put_n_chars (handle, precision, '0'))
                            return EOF;
                        /* print the actual number */
                        for (cval = *++q; cval; cval = *++q)
                            if (FSputc (cval, handle) == EOF)
                                return EOF;
                        /* if there are any spaces left, they go to right-pad
                        the field */
                        if (str_put_n_chars (handle, space_cnt, ' '))
                            return EOF;

                        count += precision + digit_cnt + space_cnt + prefix_cnt;
                    }
                    break;
                case 'n':
                    switch (size)
                    {
                        case _FMT_LONG:
                            *(long *) va_arg (ap, long *) = count;
                            break;
                        #ifdef USE_PIC18
                        case _FMT_SHRTLONG:
                            *(short long *) va_arg (ap, short long *) = count;
                            break;
                        #else
                        case _FMT_LONGLONG:
                            *(long long *) va_arg (ap, long long *) = count;
                            break;
                        #endif
                        case _FMT_BYTE:
                            *(signed char *) va_arg (ap, signed char *) = count;
                            break;
                        default:
                            *(int *) va_arg (ap, int *) = count;
                            break;
                    }
                    break;
                default:
                    /* undefined behaviour. we do nothing */
                    break;
            }
        }
        else
        {
            if (FSputc (c, handle) == EOF)
                return EOF;
            ++count;
        }
    }
    return count;
}

int FSputc (char c, FSFILE * f)
{
    if (FSfwrite ((void *)&c, 1, 1, f) != 1)
        return EOF;
    else
        return 0;
}


unsigned char str_put_n_chars (FSFILE * handle, unsigned char n, char c)
{
    while (n--)
    if (FSputc (c, handle) == EOF)
        return 1;
    return 0;
}

#endif





