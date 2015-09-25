/******************************************************************************
 *
 *                Microchip Memory Disk Drive File System
 *
 ******************************************************************************
 * FileName:        SD-SPI.h
 * Dependencies:    GenericTypeDefs.h
 *					FSconfig.h
 *					FSDefs.h
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

#ifndef SDMMC_H
#define	SDMMC_H

#define USE_SDMMC

#include "GenericTypeDefs.h"
#include "FSconfig.h"
#include "FSDefs.h"


/********************************************************************/
/*                Pin and register definitions                      */
/********************************************************************/

#define SD_CS				PORTGbits.RG9
#define SD_CS_TRIS			TRISGbits.TRISG9
#define SD_CD				0 //PORTFbits.RF0
#define SD_CD_TRIS			0 //TRISFbits.TRISF0
#define SD_WE				0 //PORTFbits.RF1
#define SD_WE_TRIS			0 //TRISFbits.TRISF1


// Registers for the SPI module you want to use
//#define SPICON1				SPI1CON1
//#define SPISTAT				SPI1STAT
//#define SPIBUF				SPI1BUF
//#define SPISTAT_RBF			SPI1STATbits.SPIRBF
//#define SPICON1bits			SPI1CON1bits
//#define SPISTATbits			SPI1STATbits

#define SPICON1				SPI2CON1
#define SPISTAT				SPI2STAT
#define SPIBUF				SPI2BUF
#define SPISTAT_RBF			SPI2STATbits.SPIRBF
#define SPICON1bits			SPI2CON1bits
#define SPISTATbits			SPI2STATbits


// Tris pins for SCK/SDI/SDO lines
#define SPICLOCK			TRISGbits.TRISG6
#define SPIIN				TRISGbits.TRISG7
#define SPIOUT			    TRISGbits.TRISG8


#ifdef USE_PIC18
	#define   SYNC_MODE_FAST	0x00
	#define   SYNC_MODE_SLOW	0x02 
	#define   BUS_MODE			0 
	#define   SMP_PHASE			0x80
#else
	#define   SYNC_MODE_FAST	0x3E
	#define   SYNC_MODE_SLOW	0x20 
	#define   BUS_MODE			3    
	#define   SMP_PHASE			0 	
#endif



/*****************************************************************/
/*                  Strcutures and defines                       */
/*****************************************************************/


/* Command Operands */
#define BLOCKLEN_64                 0x0040
#define BLOCKLEN_128                0x0080
#define BLOCKLEN_256                0x0100
#define BLOCKLEN_512                0x0200

/* Data Token */
#define DATA_START_TOKEN            0xFE
#define DATA_MULT_WRT_START_TOK     0xFC
#define DATA_MULT_WRT_STOP_TOK      0xFD

/* Data Response */
#define DATA_ACCEPTED               0b00000101
#define DATA_CRC_ERR                0b00001011
#define DATA_WRT_ERR                0b00001101

#define MMC_Interrupt   *IntReg

#define MOREDATA    !0
#define NODATA      0


#define MMC_FLOATING_BUS    0xFF
#define MMC_BAD_RESPONSE    MMC_FLOATING_BUS
#define MMC_ILLEGAL_CMD     0x04
#define MMC_GOOD_CMD        0x00

// The SDMMC Commands
#define     cmdGO_IDLE_STATE        0
#define     cmdSEND_OP_COND         1        
#define     cmdSEND_CSD             9
#define     cmdSEND_CID             10
#define     cmdSTOP_TRANSMISSION    12
#define     cmdSEND_STATUS          13
#define     cmdSET_BLOCKLEN         16
#define     cmdREAD_SINGLE_BLOCK    17
#define     cmdREAD_MULTI_BLOCK     18
#define     cmdWRITE_SINGLE_BLOCK   24    
#define     cmdWRITE_MULTI_BLOCK    25
#define     cmdTAG_SECTOR_START     32
#define     cmdTAG_SECTOR_END       33
#define     cmdUNTAG_SECTOR         34
#define     cmdTAG_ERASE_GRP_START  35 
#define     cmdTAG_ERASE_GRP_END    36
#define     cmdUNTAG_ERASE_GRP      37
#define     cmdERASE                38
#define     cmdLOCK_UNLOCK          49
#define     cmdSD_APP_OP_COND       41
#define     cmdAPP_CMD              55
#define     cmdREAD_OCR             58
#define     cmdCRC_ON_OFF           59

// the various possible responses
typedef enum
{
    R1,
    R1b,
    R2,
    R3    // we don't use R3 since we don't care about OCR 
}RESP;

// The various command informations needed 
typedef struct
{
    BYTE      CmdCode;            // the command number
    BYTE      CRC;            // the CRC value (CRC's are not required once you turn the option off!)
    RESP    responsetype;   // the Response Type
    BYTE    moredataexpected;   // True if more data is expected
} typMMC_CMD;

typedef union
{
    struct
    {
		#ifdef USE_PIC18
	        BYTE field[6];
		#else
			BYTE field[7];
		#endif
    };
    struct
    {
        BYTE crc;
		#ifndef USE_PIC18
			BYTE c30filler;	// This is here because 1 bit field cant cross an int boundry
		#endif
        BYTE addr0;
        BYTE addr1;
        BYTE addr2;
        BYTE addr3;
        BYTE cmd;
    };
    struct
    {
        unsigned END_BIT:1;
        unsigned CRC7:7;
        DWORD address;
        unsigned CMD_INDEX:6;
        unsigned TRANSMIT_BIT:1;
        unsigned START_BIT:1;
    };
} CMD_PACKET;

typedef union
{
    BYTE _byte;
    struct
    {
        unsigned IN_IDLE_STATE:1;
        unsigned ERASE_RESET:1;
        unsigned ILLEGAL_CMD:1;
        unsigned CRC_ERR:1;
        unsigned ERASE_SEQ_ERR:1;
        unsigned ADDRESS_ERR:1;
        unsigned PARAM_ERR:1;
        unsigned B7:1;
    };
} RESPONSE_1;

typedef union
{
    WORD _word;
    struct
    {
        BYTE      _byte0;
        BYTE      _byte1;
    };
    struct
    {
        unsigned IN_IDLE_STATE:1;
        unsigned ERASE_RESET:1;
        unsigned ILLEGAL_CMD:1;
        unsigned CRC_ERR:1;
        unsigned ERASE_SEQ_ERR:1;
        unsigned ADDRESS_ERR:1;
        unsigned PARAM_ERR:1;
        unsigned B7:1;
        unsigned CARD_IS_LOCKED:1;
        unsigned WP_ERASE_SKIP_LK_FAIL:1;
        unsigned ERROR:1;
        unsigned CC_ERROR:1;
        unsigned CARD_ECC_FAIL:1;
        unsigned WP_VIOLATION:1;
        unsigned ERASE_PARAM:1;
        unsigned OUTRANGE_CSD_OVERWRITE:1;
    };
} RESPONSE_2;

typedef union
{
    RESPONSE_1  r1;  
    RESPONSE_2  r2;
}MMC_RESPONSE;


typedef union
{
    struct
    {
        DWORD _u320;
        DWORD _u321;
        DWORD _u322;
        DWORD _u323;
    };
    struct
    {
        BYTE _byte[16];
    };
    struct
    {
        unsigned NOT_USED           :1;
        unsigned CRC                :7; //bit 000 - 007
        
        unsigned ECC                :2;
        unsigned FILE_FORMAT        :2;
        unsigned TMP_WRITE_PROTECT  :1;
        unsigned PERM_WRITE_PROTECT :1;
        unsigned COPY               :1;
        unsigned FILE_FORMAT_GRP    :1; //bit 008 - 015
        
        unsigned RESERVED_1         :5;
        unsigned WRITE_BL_PARTIAL   :1;
        unsigned WRITE_BL_LEN_L     :2;
        
        unsigned WRITE_BL_LEN_H     :2;
        unsigned R2W_FACTOR         :3;
        unsigned DEFAULT_ECC        :2;
        unsigned WP_GRP_ENABLE      :1; //bit 016 - 031
        
        unsigned WP_GRP_SIZE        :5;
        unsigned ERASE_GRP_SIZE_L   :3;
        
        unsigned ERASE_GRP_SIZE_H   :2;
        unsigned SECTOR_SIZE        :5;
        unsigned C_SIZE_MULT_L      :1;
        
        unsigned C_SIZE_MULT_H      :2;
        unsigned VDD_W_CURR_MAX     :3;
        unsigned VDD_W_CUR_MIN      :3;
        
        unsigned VDD_R_CURR_MAX     :3;
        unsigned VDD_R_CURR_MIN     :3;
        unsigned C_SIZE_L           :2;
        
        unsigned C_SIZE_H           :8;
        
        unsigned C_SIZE_U           :2;
        unsigned RESERVED_2         :2;
        unsigned DSR_IMP            :1;
        unsigned READ_BLK_MISALIGN  :1;
        unsigned WRITE_BLK_MISALIGN :1;
        unsigned READ_BL_PARTIAL    :1;
        
        unsigned READ_BL_LEN        :4;
        unsigned CCC_L              :4;
        
        unsigned CCC_H              :8;
        
        unsigned TRAN_SPEED         :8;
        
        unsigned NSAC               :8;
        
        unsigned TAAC               :8;
        
        unsigned RESERVED_3         :2;
        unsigned SPEC_VERS          :4;
        unsigned CSD_STRUCTURE      :2;
    };
} CSD;


typedef union
{
    struct
    {
        DWORD _u320;
        DWORD _u321;
        DWORD _u322;
        DWORD _u323;
    };
    struct
    {
        BYTE _byte[16];
    };
    struct
    {
        unsigned 	NOT_USED           	:1;
        unsigned 	CRC                	:7;     
        unsigned 	MDT                	:8;     //Manufacturing Date Code (BCD)
        DWORD 		PSN;    					// Serial Number (PSN)
        unsigned 	PRV                	:8;     // Product Revision
		char		PNM[6];    					// Product Name
        WORD 		OID;    					// OEM/Application ID
        unsigned 	MID                	:8;     // Manufacture ID                        
    };
} CID;

#define FALSE	0
#define TRUE	!FALSE

#define INPUT	1
#define OUTPUT	0



#ifdef USE_PIC18
	#define CLKSPERINSTRUCTION (BYTE) 4
#else
	#define CLKSPERINSTRUCTION (BYTE) 2
#endif
#define TMR1PRESCALER	(BYTE)    8
#define TMR1OVERHEAD	(BYTE)    5	
#define MILLISECDELAY   (WORD)((SYSTEM_CLOCK/CLKSPERINSTRUCTION/TMR1PRESCALER/(WORD)1000) - TMR1OVERHEAD)
#define B115K26MHZ      0x0C        // = 115.2K baud @26MHz

#define SD_CMD_IDLE				0
#define SD_CMD_SEND_OP_COND		1
#define SD_CMD_SET_BLOCK_LEN	16
#define SD_CMD_READ_BLOCK		17
#define SD_CMD_WRITE_BLOCK		24

#define SD_CARD_SYSTEM

typedef enum
{
	GO_IDLE_STATE,
	SEND_OP_COND,
	SEND_CSD,
	SEND_CID,
	STOP_TRANSMISSION,
	SEND_STATUS,
	SET_BLOCKLEN,
	READ_SINGLE_BLOCK,
	READ_MULTI_BLOCK,
	WRITE_SINGLE_BLOCK,
	WRITE_MULTI_BLOCK,
	TAG_SECTOR_START,
	TAG_SECTOR_END,
	UNTAG_SECTOR,
	TAG_ERASE_GRP_START,
	TAG_ERASE_GRP_END,
	UNTAG_ERASE_GRP,
	ERASE,
	LOCK_UNLOCK,
	SD_APP_OP_COND,
	APP_CMD,
	READ_OCR,
	CRC_ON_OFF
}sdmmc_cmd;



/***************************************************************************/
/*                               Macros                                    */
/***************************************************************************/
#define MMC_OFF		// Power "Off" MMC Slot if available
#define MMC_ON		// Power "On" MMC Slot if available
#define GetMMC_CD()	((*MMCReg & MMC_DETECT) == MMC_DETECT ? FALSE : TRUE)


#define mSendMediaCmd_NoData()  SendMediaCmd();SD_CS=1;
#define mReadCRC()              WriteSPIM(0xFF);WriteSPIM(0xFF);
#define mSendCRC()              WriteSPIM(0xFF);WriteSPIM(0xFF);
#define mSend8ClkCycles()       WriteSPIM(0xFF);

#define low(num) (num & 0xFF)
#define high(num) ((num >> 8) & 0xFF)
#define upper(num) ((num >> 16) & 0xFF)

#define MediaIsPresent()			(!SD_CD)
#define MediaIsWriteProtected()		(SD_WE)



/*****************************************************************************/
/*                                 Prototypes                                */
/*****************************************************************************/

DWORD ReadCapacity(void);
WORD ReadSectorSize(void);
void InitIO(void);
BYTE MediaDetect(void);
BYTE MediaInitialize(void);
BYTE SectorRead(DWORD sector_addr, BYTE* buffer);
BYTE SectorWrite(DWORD sector_addr, BYTE* buffer, BYTE allowWriteToZero);
BYTE WriteProtectState(void);
void ShutdownMedia(void);

#ifdef USE_PIC24
	BYTE ReadByte( BYTE* pBuffer, WORD index );
	WORD ReadWord( BYTE* pBuffer, WORD index );
	DWORD ReadDWord( BYTE* pBuffer, WORD index );
#endif

#endif
