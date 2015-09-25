/******************************************************************************
 *
 *               Microchip Memory Disk Drive File System
 *
 ******************************************************************************
 * FileName:        FSDefs.h
 * Dependencies:    GenericTypeDefs.h
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

#ifndef  _FSDEF__H
#define  _FSDEF__H

#include "GenericTypeDefs.h"

// ERROR CODES
typedef enum _CETYPE
{
	CE_GOOD = 0,					 // Everything is fine
	CE_ERASE_FAIL,					 // Internal Card erase failed
	CE_NOT_PRESENT,					 // CARD not present
	CE_NOT_FORMATTED,				 // The disk is of an unsupported format
	CE_BAD_PARTITION,				 // The boot record is bad
	CE_INIT_ERROR,					 // Initialization error has occured
	CE_NOT_INIT,					 // Card is not yet initialized because of some error
	CE_BAD_SECTOR_READ,				 // A bad read occured of a sector
	CE_WRITE_ERROR,					 // Could not write to the sector
	CE_INVALID_CLUSTER,				 // invalid cluster value > maxcls
	CE_FILE_NOT_FOUND,				 // Could not find the file on the card
	CE_DIR_NOT_FOUND,				 // Could not find the directory
	CE_BAD_FILE,					 // file is corrupted
	CE_DONE,						 // No more files in this directory
	CE_FILENAME_2_LONG,				 // The purposed file name is too long to use. Shorten the name and then resend.
	CE_FILENAME_EXISTS,				 // This filename already exists
	CE_NO_MORE_TAILS,				 // Long file name could not be created
	CE_DIR_FULL,					 // all root dir entry are taken
	CE_DISK_FULL,					 // all clusters in partition are taken
	CE_DIR_NOT_EMPTY,				 // This directory is not empty yet, remove files b4 deleting
	CE_NONSUPPORTED_SIZE,			 // The disk is too big to format as FAT16
	CE_TEMP_MOUNT_FAILED,			 // A temporary mount failed (format)
	CE_WRITE_PROTECTED,				 // Card is write protected
	CE_UNLOCK_FAILED,				 // Card unlock failed
	CE_INVALIDCSUM,					 // Invalid checksum
	CE_ENVLOADFAIL,					 // The environment failed to load
	CE_TMPMEMFULL,					 // No more external RAM
	CE_FILENOTOPENED,				 // File not openned for the write
	CE_BADCACHEREAD,				 // Bad cache read
	CE_CARDFAT32,					 // FAT 32 - card not supported
	CE_IMAGENOTAVAIL,				 // The PC has tried to tell me to use a SQTP file without a part selected
	CE_READONLY,					 // The File is readonly
	CE_CARDFAT12,					 // FAT12 during intial testing we are not supporting FAT12
	CE_WRITEONLY					 // The File is open in write only mode - a read was attempted
} CETYPE;

#define FOUND       0       // directory entry match
#define ERASED      0       // An erase occured correctly
#define NOT_FOUND   1       // directory entry not found
#define NO_MORE     2       // no more files found
#define WRITE_ERROR 3       // a write error occured

#define FAT12       1       // internal flags for FAT type 12 and 16
#define FAT16       2
#define FAT32       3

#define ATTR_READ_ONLY      0x01         
#define ATTR_HIDDEN         0x02           
#define ATTR_SYSTEM         0x04           
#define ATTR_VOLUME         0x08            
#define ATTR_LONG_NAME      0x0f                          
#define ATTR_EXTEND         0x0f 
#define ATTR_DIRECTORY      0x10            
#define ATTR_ARCHIVE        0x20       
#define ATTR_MASK			0x3f     


#define FILT_READ_ONLY      0x01         
#define FILT_HIDDEN         0x02           
#define FILT_SYSTEM         0x04           
#define FILT_VOLUME         0x08        
#define FILT_EXTND          (FILT_VOLUME|FILT_SYSTEM|FILT_HIDDEN|FILT_READ_ONLY)
#define FILT_DIRECTORY      0x10                
#define FILT_NORMAL_FILE    0x20   
#define FILT_MASK           (0x3F)       

#define CLUSTER_EMPTY       0x0000
#define LAST_CLUSTER_FAT12  0xff8
#define LAST_CLUSTER_FAT16  0xfff8
#define LAST_CLUSTER        0xfff8
#define END_CLUSTER         0xFFFE
#define CLUSTER_FAIL        0xFFFF
#define WIN_LAST_CLUS       0xFFFF
        
#define FAT_ENTRY0          0xFFF8
#define FAT_ENTRY1          0xFFFF

#define DIR_DEL             0xE5    // marker of a deleted entry
#define DIR_EMPTY           0       // marker of last entry in directory

#define DIR_NAMESIZE        8
#define DIR_EXTENSION       3
#define DIR_NAMECOMP        (DIR_NAMESIZE+DIR_EXTENSION)

#define RAMwrite( a, f, d) *(a+f) = d
#define RAMread( a, f)  *(a+f)
#define RAMreadW( a, f) *(WORD *)(a+f)
#define RAMreadD( a, f) *(DWORD *)(a+f)

#include <stdio.h>
#ifndef EOF
	#define EOF         		(-1)
#endif

typedef struct 
{ 
    BYTE*   	buffer;         // address of sector buffer in ext RAM
    DWORD     	firsts;         // lba of first sector in (first) partition
    DWORD     	fat;            // lba of FAT
    DWORD     	root;           // lba of root directory
    DWORD     	data;           // lba of the data area 
    WORD     	maxroot;        // max number of entries in root dir
    DWORD     	maxcls;         // max number of clusters in partition
    WORD     	fatsize;        // number of sectors
    BYTE      	fatcopy;        // number of copies
    BYTE      	SecPerClus;     // number of sectors per cluster
    BYTE      	type;           // type of FAT (FAT12, FAT16...)
    BYTE      	mount;          // flag (TRUE= mounted, FALSE= invalid)    
} DISK;

#ifdef USE_PIC18
    typedef unsigned short long SWORD;
#else
	typedef struct
	{
		unsigned char array[3];
	} SWORD;	
#endif

// BPB FAT12
typedef struct __BPB_FAT12 {
        SWORD BootSec_JumpCmd;        // Jump Command
        BYTE  BootSec_OEMName[8];     // OEM name
        WORD BootSec_BPS;     // BYTEs per sector
        BYTE  BootSec_SPC;     // sectors per allocation unit
        WORD BootSec_ResrvSec;     // number of reserved sectors after start
        BYTE  BootSec_FATCount;        // number of FATs
        WORD BootSec_RootDirEnts;     // number of root directory entries
        WORD BootSec_TotSec16;       // total number of sectors
        BYTE  BootSec_MDesc;          // media descriptor
        WORD BootSec_SPF;         // number of sectors per FAT
        WORD BootSec_SPT;      // sectors per track
        WORD BootSec_HeadCnt;       // number of heads
        DWORD BootSec_HiddenSecCnt;        // number of hidden sectors
        DWORD  BootSec_Reserved;       // Nothing
        BYTE  BootSec_DriveNum;          // Int 13 drive number
        BYTE  BootSec_Reserved2;       // Nothing
        BYTE  BootSec_BootSig;         // 0x29
        BYTE  BootSec_VolID[4];        // Volume Id
        BYTE  BootSec_VolLabel[11];       // Volume Label
        BYTE  BootSec_FSType[8];   // File system type, not used for determination   
}_BPB_FAT12;

typedef _BPB_FAT12 * BPB_FAT12;

// BPB FAT16
typedef struct __BPB_FAT16 {
        SWORD BootSec_JumpCmd;        // Jump Command
        BYTE  BootSec_OEMName[8];     // OEM name
        WORD  BootSec_BPS;     // BYTEs per sector
        BYTE  BootSec_SPC;     // sectors per allocation unit
        WORD  BootSec_ResrvSec;     // number of reserved sectors after start
        BYTE  BootSec_FATCount;        // number of FATs
        WORD  BootSec_RootDirEnts;     // number of root directory entries
        WORD  BootSec_TotSec16;       // total number of sectors
        BYTE  BootSec_MDesc;          // media descriptor
        WORD  BootSec_SPF;         // number of sectors per FAT
        WORD  BootSec_SPT;      // sectors per track
        WORD  BootSec_HeadCnt;       // number of heads
        DWORD BootSec_HiddenSecCnt;        // number of hidden sectors
        DWORD BootSec_TotSec32;       // 32bit total sec count
        BYTE  BootSec_DriveNum;          // Int 13 drive number
        BYTE  BootSec_Reserved;       // Nothing
        BYTE  BootSec_BootSig;         // 0x29
        BYTE  BootSec_VolID[4];        // Volume Id
        BYTE  BootSec_VolLabel[11];       // Volume Label
        BYTE  BootSec_FSType[8];   // File system type, not used for determination     
}_BPB_FAT16;

#define BSI_JMPCMD         0
#define BSI_OEMNAME        3
#define BSI_BPS            11
#define BSI_SPC            13
#define BSI_RESRVSEC       14
#define BSI_FATCOUNT         16
#define BSI_ROOTDIRENTS    17
#define BSI_TOTSEC16       19
#define BSI_MDESC          21
#define BSI_SPF            22
#define BSI_SPT            24
#define BSI_HEADCNT        26
#define BSI_HIDDENSECCNT   28
#define BSI_TOTSEC32       32
#define BSI_DRIVENUM       36
#define BSI_RESERVED       37
#define BSI_BOOTSIG        38
#define BSI_VOLID          39
#define BSI_VOLLABEL       43
#define BSI_FSTYPE         54


typedef _BPB_FAT16 * BPB_FAT16;

// PTE_MBR - Partition Table Entry
typedef struct _PTE_MBR
{
    BYTE      PTE_BootDes;            // Boot Descriptor, 0x80
    SWORD     PTE_FrstPartSect;       // First Partion Sector
    BYTE      PTE_FSDesc;             // File System Descriptor 
    SWORD     PTE_LstPartSect;        // Last Partion Sector
    DWORD     PTE_FrstSect;           // First Sector Position
    DWORD     PTE_NumSect;            // Number of Sectors in partion
}PTE_MBR;

// PT_MBR - Partition Table 
typedef struct __PT_MBR
{
    BYTE      ConsChkRtn[446];
    PTE_MBR Partition0;
    PTE_MBR Partition1;
    PTE_MBR Partition2;
    PTE_MBR Partition3;
    BYTE      Signature0;     // 0x55
    BYTE      Signature1;     // 0xAA
}_PT_MBR;

typedef _PT_MBR *  PT_MBR;

typedef struct __BootSec
{
    union
    {
        _BPB_FAT16  FAT_16;	
        _BPB_FAT12  FAT_12;	
    }FAT;    
    BYTE  	Reserved[MEDIA_SECTOR_SIZE-sizeof(_BPB_FAT16)-2];
    BYTE      Signature0;     // 0x55
    BYTE      Signature1;     // 0xAA
}_BootSec;

typedef _BootSec * BootSec;

// Master Boot Record offsets
#define FO_MBR          0L  // master boot record sector LBA

#ifndef EOF
    #define EOF         (-1)
#endif

#define FAT_GOOD_SIGN_0     0x55
#define FAT_GOOD_SIGN_1     0xAA
#define BOOTABLE            0x80
#define NON_BOOTABLE        0x00



#endif
