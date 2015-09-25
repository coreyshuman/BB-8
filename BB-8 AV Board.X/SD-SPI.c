/******************************************************************************
 *
 *               Microchip Memory Disk Drive File System
 *
 ******************************************************************************
 * FileName:        SD-SPI.c
 * Dependencies:    SD-SPI.h
 *               string.h
 *               FSIO.h
 *                  FSDefs.h
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

#include "FSIO.h"
#include "FSDefs.h"
#include "SD-SPI.h"
#include "string.h"
#include "FSConfig.h"

/******************************************************************************
 * Global Variables
 *****************************************************************************/

DWORD finalLBA;


/*********************************************************************
 * sdmmc_cmdtable
 * - Provides information for all the sdmmc commands that we support
 * 
 * Notes: We turn off the CRC as soon as possible, so the commands with
 *        0xFF don't need to be calculated in runtime 
 *********************************************************************/
#ifdef USE_PIC18
    const rom typMMC_CMD sdmmc_cmdtable[] =
#else
    const typMMC_CMD sdmmc_cmdtable[] =
#endif
{
    // cmd                      crc     response
    {cmdGO_IDLE_STATE,          0x95,   R1,     NODATA},
    {cmdSEND_OP_COND,           0xF9,   R1,     NODATA},
    {cmdSEND_CSD,               0xAF,   R1,     MOREDATA},
    {cmdSEND_CID,               0x1B,   R1,     MOREDATA},
    {cmdSTOP_TRANSMISSION,      0xC3,   R1,     NODATA},
    {cmdSEND_STATUS,            0xAF,   R2,     NODATA},
    {cmdSET_BLOCKLEN,           0xFF,   R1,     NODATA},
    {cmdREAD_SINGLE_BLOCK,      0xFF,   R1,     MOREDATA},
    {cmdREAD_MULTI_BLOCK,       0xFF,   R1,     MOREDATA},
    {cmdWRITE_SINGLE_BLOCK,     0xFF,   R1,     MOREDATA},
    {cmdWRITE_MULTI_BLOCK,      0xFF,   R1,     MOREDATA}, 
    {cmdTAG_SECTOR_START,       0xFF,   R1,     NODATA},
    {cmdTAG_SECTOR_END,         0xFF,   R1,     NODATA},
    {cmdUNTAG_SECTOR,           0xFF,   R1,     NODATA},
    {cmdTAG_ERASE_GRP_START,    0xFF,   R1,     NODATA},
    {cmdTAG_ERASE_GRP_END,      0xFF,   R1,     NODATA},
    {cmdUNTAG_ERASE_GRP,        0xFF,   R1,     NODATA},
    {cmdERASE,                  0xDF,   R1b,    NODATA},
    {cmdLOCK_UNLOCK,            0x89,   R1b,    NODATA},  
    {cmdSD_APP_OP_COND,         0xE5,   R1,     NODATA},
    {cmdAPP_CMD,                0x73,   R1,     NODATA},
    {cmdREAD_OCR,               0x25,   R3,     NODATA},
    {cmdCRC_ON_OFF,             0x25,   R1,     NODATA}
};




/******************************************************************************
 * Prototypes
 *****************************************************************************/

extern void Delayms(BYTE milliseconds);
BYTE ReadMedia(void);
BYTE MediaInitialize(void);
MMC_RESPONSE SendMMCCmd(BYTE cmd, DWORD address);

#ifdef USE_PIC24
    void OpenSPIM ( unsigned int sync_mode, unsigned char bus_mode, unsigned char smp_phase);
    void CloseSPIM( void );
    unsigned char WriteSPIM( unsigned char data_out );
#elif defined USE_PIC18
    void OpenSPIM ( unsigned char sync_mode, unsigned char bus_mode, unsigned char smp_phase);
    void CloseSPIM( void );
    unsigned char WriteSPIM( unsigned char data_out );
#endif

/******************************************************************************
 * Function:        BYTE MediaDetect(void)
 *
 * PreCondition:    InitIO() function has been executed.
 *
 * Input:           void
 *
 * Output:          TRUE   - Card detected
 *                  FALSE   - No card detected
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 *****************************************************************************/
BYTE MediaDetect()
{
    return(!SD_CD);
}//end MediaDetect

/******************************************************************************
 * Function:        WORD ReadSectorSize(void)
 *
 * PreCondition:    MediaInitialize() is complete
 *
 * Input:           void
 *
 * Output:          WORD - size of the sectors for this physical media.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 *****************************************************************************/
WORD ReadSectorSize(void)
{
    return MEDIA_SECTOR_SIZE;
}

/******************************************************************************
 * Function:        DWORD ReadCapacity(void)
 *
 * PreCondition:    MediaInitialize() is complete
 *
 * Input:           void
 *
 * Output:          DWORD - size of the "disk"
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 *****************************************************************************/
DWORD ReadCapacity()
{
    return (finalLBA);
}

/******************************************************************************
 * Function:        void InitIO(void)
 *
 * PreCondition:    None
 *
 * Input:           void
 *
 * Output:          void
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 *****************************************************************************/

void InitIO (void)
{
    // Turn off the card
    MMC_OFF;
//    SD_CD_TRIS = INPUT;            //Card Detect - input
    SD_CS = 1;                     //Initialize Chip Select line
    SD_CS_TRIS = OUTPUT;            //Card Select - output
//    SD_WE_TRIS = INPUT;            //Write Protect - input
}
      

/******************************************************************************
 * Function:        BYTE MediaInitialize(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          TRUE                    - Everything is fine
 *                  FALSE      - Communication was not established
 *
 * Overview:        MediaInitialize initializes the media card and supporting variables.
 *
 * Note:            
 *****************************************************************************/
BYTE MediaInitialize(void)
{
    WORD timeout;
    BYTE       status = TRUE;
    MMC_RESPONSE    response; 
	int Retries = 100;
    
    SD_CS = 1;               //Initialize Chip Select line
    
    MMC_ON;
    
    //Media powers up in the open-drain mode and cannot handle a clock faster
    //than 400kHz. Initialize SPI port to slower than 400kHz
#ifdef USE_PIC24
    SPICON1 = 0x0000;                // power on state
    SPICON1 |= SYNC_MODE_SLOW;
#endif
    OpenSPIM(SYNC_MODE_SLOW, BUS_MODE, SMP_PHASE);
    
    // let the card power on and initialize
    Delayms(100);
    
    //Media requires 80 clock cycles to startup [8 clocks/BYTE * 10 us]
    for(timeout=0; timeout<10; timeout++)
        mSend8ClkCycles();

    SD_CS = 0;
    
    Delayms(1);
    
    // Send CMD0 to reset the media
	do
	{
		response = SendMMCCmd(GO_IDLE_STATE,0x0);
	}while(((response.r1._byte == MMC_BAD_RESPONSE) || ((response.r1._byte & 0xF7) != 0x01)) && Retries--);
       
    if((response.r1._byte == MMC_BAD_RESPONSE) || ((response.r1._byte & 0xF7) != 0x01))
    {
        status = FALSE;      // we have not got anything back from the card
        SD_CS = 1;                               // deselect the devices
        
        // Turn it off
        MMC_OFF;
        return status;
    }

    // According to spec cmd1 must be repeated until the card is fully initialized
    timeout = 0xFFF;
    
    do
    {
        response = SendMMCCmd(SEND_OP_COND,0x0);
        timeout--;
    }while(response.r1._byte != 0x00 && timeout != 0);

    // see if it failed
    if(timeout == 0)
    {
        status = FALSE;      // we have not got anything back from the card
        
        SD_CS = 1;                               // deselect the devices
        
        // Turn it off
        MMC_OFF;
    }
    else      
    {
//        OpenSPIM(SYNC_MODE_FAST, BUS_MODE, SMP_PHASE);
#ifdef USE_PIC24
/*        if (SYSTEM_CLOCK <= 20000000)
        {
            // clock < 20k -> Fcy < 10k (Max SPI speed is 10k)
            SPICON1 |= 0x3F;
        }
        else
        {
            // Divide SPI clock by 2 so it's less than 10k
            SPICON1 |= 0x3B;
        }
*/
			// Added - josh hintze

			// You can modify these up if your SD card does not work at the higher speeds
			SPISTATbits.SPIEN = 0;
			SPICON1bits.SPRE = 4;		// secondary prescalar 4:1
			SPICON1bits.PPRE = 3;		// primary prescalar 1:1
			SPISTATbits.SPIEN = 1;             // enable synchronous serial port    

#endif
        // Turn off CRC7 if we can, might be an invalid cmd on some cards (CMD59)
        response = SendMMCCmd(CRC_ON_OFF,0x0);
        
        // Now set the block length to media sector size. It should be already
        SendMMCCmd(SET_BLOCKLEN,MEDIA_SECTOR_SIZE);
        
        for(timeout = 0xFF; timeout > 0 && SectorRead(0x0,NULL) != TRUE; timeout--)
        {;}

        // see if we had an issue
        if(timeout == 0)
        {
            status = FALSE;
            SD_CS = 1;                               // deselect the devices
            
            // Turn it off
            MMC_OFF;
        }
    }

    return(status);
}//end MediaInitialize

/******************************************************************************
 * Function:        void ShutdownMedia(void)
 *
 * PreCondition:    None
 *
 * Input:           void
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Turn off the card
 *
 * Note:            None
 *****************************************************************************/

void ShutdownMedia(void)
{
    // close the spi bus
    CloseSPIM();
    
    // deselect the device
    SD_CS = 1;
    
    // Turn off the card
    MMC_OFF;
}

/******************************************************************************
 * Function:        void SendMMCCmd(BYTE cmd, DWORD address)
 *
 *
 * Input:           None
 *
 * Output:          response            - Response from the card
 *                                      - 0x00 or 0x01 Command received 
 *                                        successfully, else, command failed
 *                  -Bit 0              - In idle state if 1
 *                  -Bit 1              - Erase Reset if 1
 *                  -Bit 2              - Illgal Command if 1
 *                  -Bit 3              - Com CRC Error if 1
 *                  -Bit 4              - Erase Sequence Error if 1
 *                  -Bit 5              - Address Error if 1
 *                  -Bit 6              - Parameter Error if 1
 *                  -Bit 7              - Not used, always '0'
 *
 * Side Effects:    None
 *
 * Overview:        SendMMCCmd prepares the command packet and sends it out
 *                  over the SPI interface. Response data of type 'R1' (see
 *                  SD or MMC product manuals) is returned.
 *
 * Note:            MMC_CS is not set at the end of this function.
 *                  if the command has no data stage, call macro
 *                  mSendMediaCmd_NoData(), it reasserts MMC_CS to 1.
 *                  If the command has a data stage, MMC_CS must be
 *                  reasserted after the data transfer stage is complete.
 *                  See SectorRead and SectorWrite for examples.
 *****************************************************************************/
MMC_RESPONSE SendMMCCmd(BYTE cmd, DWORD address)
{
    WORD timeout = 0x8;
    BYTE index;
    MMC_RESPONSE    response;
    CMD_PACKET  CmdPacket;
    
    SD_CS = 0;                           //Card Select
    
    // Copy over data
    CmdPacket.cmd        = sdmmc_cmdtable[cmd].CmdCode;
    CmdPacket.address    = address;
    CmdPacket.crc        = sdmmc_cmdtable[cmd].CRC;       // Calc CRC here
    
    CmdPacket.TRANSMIT_BIT = 1;             //Set Tranmission bit
    
    WriteSPIM(CmdPacket.cmd);                //Send Command
    WriteSPIM(CmdPacket.addr3);              //Most Significant Byte
    WriteSPIM(CmdPacket.addr2);
    WriteSPIM(CmdPacket.addr1);
    WriteSPIM(CmdPacket.addr0);              //Least Significant Byte
    WriteSPIM(CmdPacket.crc);                //Send CRC
    
    // see if we are going to get a response
    if(sdmmc_cmdtable[cmd].responsetype == R1 || sdmmc_cmdtable[cmd].responsetype == R1b)
    {
        do
        {
            response.r1._byte = ReadMedia();
            timeout--;
        }while(response.r1._byte == MMC_FLOATING_BUS && timeout != 0);
    }
    else if(sdmmc_cmdtable[cmd].responsetype == R2)
    {
        ReadMedia();
        
        response.r2._byte1 = ReadMedia();
        response.r2._byte0 = ReadMedia();
    }

    if(sdmmc_cmdtable[cmd].responsetype == R1b)
    {
        response.r1._byte = 0x00;
        
        for(index =0; index < 0xFF && response.r1._byte == 0x00; index++)
        {
            timeout = 0xFFFF;
            
            do
            {
                response.r1._byte = ReadMedia();
                timeout--;
            }while(response.r1._byte == 0x00 && timeout != 0);
        }
    }

    mSend8ClkCycles();                      //Required clocking (see spec)

    // see if we are expecting data or not
    if(!(sdmmc_cmdtable[cmd].moredataexpected))
        SD_CS = 1;

    return(response);
}


/******************************************************************************
 * Function:        BYTE SectorRead(DWORD sector_addr, BYTE *buffer)
 *
 * PreCondition:    None
 *
 * Input:           sector_addr - Sector address, each sector contains 512-byte
 *                  buffer      - Buffer where data will be stored, if NULL do
 *                                not store the data anywhere
 *
 * Output:          TRUE      - Sector read
 *               FALSE      - Sector could not be read
 *
 * Side Effects:    None
 *
 * Overview:        SectorRead reads 512 bytes of data from the card starting
 *                  at the sector address specified by sector_addr and stores
 *                  them in the location pointed to by 'buffer'.
 *
 * Note:            The card expects the address field in the command packet
 *                  to be a byte address. Therefore the sector_addr must first
 *                  be converted to byte address. This is accomplished by
 *                  shifting the address left 9 times.
 *****************************************************************************/
BYTE SectorRead(DWORD sector_addr, BYTE* buffer)
{
    WORD index;
    WORD delay;
    MMC_RESPONSE    response;
    BYTE data_token;
    BYTE status = TRUE;
    DWORD   new_addr;
#ifdef USB_MSD
    BYTE temp;
    DWORD firstSector;
    DWORD numSectors;
#endif

    // send the cmd
    new_addr = sector_addr << 9;
    response = SendMMCCmd(READ_SINGLE_BLOCK,new_addr);

    // Make sure the command was accepted
    if(response.r1._byte != 0x00)
    {
        response = SendMMCCmd (READ_SINGLE_BLOCK,new_addr);
        if(response.r1._byte != 0x00)
            status = FALSE;
    }
    else
    {
        index = 0x2FF;
        
        // Timing delay- at least 8 clock cycles
        delay = 0x40;
        while (delay)
            delay--;
      
        //Now, must wait for the start token of data block
        do
        {
            data_token = ReadMedia();
            index--;   
            
            delay = 0x40;
            while (delay)
                delay--;

        }while((data_token == MMC_FLOATING_BUS) && (index != 0));

        // Hopefully that zero is the datatoken
        if((index == 0) || (data_token != DATA_START_TOKEN))
        {
            status = FALSE;
        }
        else
        {
#ifdef USB_MSD
            if ((sector_addr == 0) && (buffer == NULL))
                finalLBA = 0x00000000;
#endif

            for(index = 0; index < MEDIA_SECTOR_SIZE; index++)      //Reads in 512-byte of data
            {
                if(buffer != NULL)
                {
                    buffer[index] = ReadMedia();
                }
                else
                {
#ifdef USB_MSD
                    if (sector_addr == 0)
                    {
                        if ((index == 0x1C6) || (index == 0x1D6) || (index == 0x1E6) || (index == 0x1F6))
                        {
                            firstSector = ReadMedia();
                            firstSector |= (DWORD)ReadMedia() << 8;
                            firstSector |= (DWORD)ReadMedia() << 16;
                            firstSector |= (DWORD)ReadMedia() << 24;
                            numSectors = ReadMedia();
                            numSectors |= (DWORD)ReadMedia() << 8;
                            numSectors |= (DWORD)ReadMedia() << 16;
                            numSectors |= (DWORD)ReadMedia() << 24;
                            index += 8;
                            if ((firstSector + numSectors - 1) > finalLBA)
                            {
                                finalLBA = firstSector + numSectors - 1;
                            }
                        }
                        else
                        {
                            ReadMedia();
                        }
                    }
                    else
                        ReadMedia();
#else
                    ReadMedia();
#endif
                }
            }
            // Now ensure CRC
            mReadCRC();               //Read 2 bytes of CRC
            //status = mmcCardCRCError;
        }

        mSend8ClkCycles();            //Required clocking (see spec)
    }

    SD_CS = 1;

    return(status);
}//end SectorRead

/******************************************************************************
 * Function:        BYTE SectorWrite(DWORD sector_addr, BYTE *buffer, BYTE allowWriteToZero)
 *
 * PreCondition:    None
 *
 * Input:           sector_addr - Sector address, each sector contains 512 bytes
 *                  buffer      - Buffer that data will be read from
 *               allowWriteToZero   - allows write to the MBR sector
 *
 * Output:          TRUE      - Sector written
 *               FALSE      - Sector could not be written
 *
 * Side Effects:    None
 *
 * Overview:        SectorWrite sends 512 bytes of data from the location
 *                  pointed to by 'buffer' to the card starting
 *                  at the sector address specified by sector_addr.
 *
 * Note:            The card expects the address field in the command packet
 *                  to be byte address. Therefore the sector_addr must first
 *                  be converted to byte address. This is accomplished by
 *                  shifting the address left 9 times.
 *****************************************************************************/
BYTE SectorWrite(DWORD sector_addr, BYTE* buffer, BYTE allowWriteToZero)
{
    WORD            index;
    BYTE            data_response;
    MMC_RESPONSE    response; 
    BYTE            status = TRUE;

    if (sector_addr == 0 && allowWriteToZero == FALSE)
        status = FALSE;
    else
    {
        // send the cmd
        response = SendMMCCmd(WRITE_SINGLE_BLOCK,(sector_addr << 9));
        
        // see if it was accepted
        if(response.r1._byte != 0x00)
            status = FALSE;
        else
        {
            WriteSPIM(DATA_START_TOKEN);                 //Send data start token
            
            for(index = 0; index < MEDIA_SECTOR_SIZE; index++)                    //Send 512 bytes of data
                WriteSPIM(buffer[index]);

            // calc crc
            mSendCRC();                                 //Send 2 bytes of CRC
            
            data_response = ReadMedia();                //Read response
            
            if((data_response & 0x0F) != DATA_ACCEPTED)
            {
                status = FALSE;
            }
            else
            {
                index = 0;            //using i as a timeout counter
                
                do                  //Wait for write completion
                {
                    data_response = ReadMedia();
                    index++;
                }while((data_response == 0x00) && (index != 0));

                if(index == 0)                                  //if timeout first
                    status = FALSE;
            }

            mSend8ClkCycles();
        }

        SD_CS = 1;

    } // Not writing to 0 sector

    return(status);
} //end SectorWrite


/******************************************************************************
 * Function:        BYTE WriteProtectState(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          BYTE    - Returns the status of the "write enabled" pin
 *
 * Side Effects:    None
 *
 * Overview:        Determines if the card is write-protected
 *
 * Note:            None
 *****************************************************************************/
BYTE WriteProtectState(void)
{
    return(SD_WE);
}


/******************************************************************************
 * Function:        BYTE Delayms(void)
 *
 * PreCondition:    None
 *
 * Input:           BYTE millisecons   - Number of ms to delay
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Delays- used for SPI timing
 *
 * Note:            None.
 *****************************************************************************/

void Delayms(BYTE milliseconds)
{
    BYTE    ms;
    DWORD   count;
    
    ms = milliseconds;
    while (ms--)
    {
        count = MILLISECDELAY;
        while (count--);
    }
    Nop();
    return;
}


/******************************************************************************
 * Function:        BYTE ReadMedia(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          BYTE    - One byte of data read in from SPI port
 *
 * Side Effects:    None
 *
 * Overview:        ReadMedia reads in one byte of data while sending out 0xFF
 *
 * Note:            Could not use ReadSPI because it initializes SSPBUF to
 *                  0x00. The card expects 0xFF (see spec).
 *****************************************************************************/
BYTE ReadMedia(void)
{
    SPIBUF = 0xFF;                              //Data Out - Logic ones
    while(!SPISTAT_RBF);                     //Wait until cycle complete
    return(SPIBUF);                             //Return with byte read
}//end ReadMedia



#ifdef USE_PIC24

/******************************************************************************
 * Function:        void WriteSPIM (void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Disables the SPI module
 *
 * Note:            None
 *****************************************************************************/

void CloseSPIM( void )
{
    SPISTAT &= 0x7FFF;               // disable synchronous serial port
}


/******************************************************************************
 * Function:        void OpenSPIM( unsigned char sync_mode, unsigned char bus_mode, unsigned char smp_phase)
 *
 * PreCondition:    None
 *
 * Input:           sync_mode   - Sets synchronization
 *               bus_mode   - Sets bus operating mode
 *               smp_phase   - Set sampling phase
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Enables SPI module
 *
 * Note:            None
 *****************************************************************************/

void OpenSPIM( unsigned int sync_mode, unsigned char bus_mode, unsigned char smp_phase)
{
    SPISTAT = 0x0000;               // power on state
    SPICON1 |= smp_phase;          // select data input sample phase

    switch( bus_mode )
    {
        case 0:                       // SPI1 bus mode 0,0
            SPICON1bits.CKE = 1;       // data transmitted on rising edge
            break;
        case 2:                       // SPI1 bus mode 1,0
            SPICON1bits.CKP = 1;       // clock idle state high
            break;
        case 3:                       // SPI1 bus mode 1,1
            SPICON1bits.CKP = 1;       // clock idle state high
            SPICON1bits.CKE = 0;
            break;
        default:                      // default SPI1 bus mode 0,1
            break;
    }

    switch( sync_mode )
    {
        case 4:                       // slave mode w /SS1 enable
            SD_CS_TRIS = 1;       // define /SS1 pin as input
        case 5:                       // slave mode w/o /SS1 enable
            SPICLOCK = 1;       // define clock pin as input
            SPICON1bits.SMP = 0;       // must be cleared in slave SPI mode
            break;
        default:                      // master mode, define clock pin as output
            SPICLOCK = 0;       // define clock pin as output
            break;
    }
    
    SPIOUT = 0;                  // define SDO1 as output (master or slave)
    SPIIN = 1;                  // define SDI1 as input (master or slave)
    SPISTATbits.SPIEN = 1;             // enable synchronous serial port
}

/******************************************************************************
 * Function:        unsigned char WriteSPIM ( unsigned char data_out)
 *
 * PreCondition:    None
 *
 * Input:           data_out   - data to transmit
 *
 * Output:          0 if successful, -1 otherwise
 *
 * Side Effects:    None
 *
 * Overview:        Writes a byte on the SPI
 *
 * Note:            None
 *****************************************************************************/

unsigned char WriteSPIM( unsigned char data_out )
{
    BYTE   clear;
    SPIBUF = data_out;          // write byte to SSP1BUF register
    while( !SPISTATbits.SPIRBF ); // wait until bus cycle complete
    clear = SPIBUF;
    return ( 0 );                // return non-negative#
}

#endif

/******************************************************************************
 * Function:        BYTE ReadByte(BYTE * pBuffer, WORD index)
 *
 * PreCondition:    None
 *
 * Input:           pBuffer   - pointer to a buffer to read from
 *               index   - index in the buffer to read to
 *
 * Output:          BYTE   - the byte read
 *
 * Side Effects:    None
 *
 * Overview:        Reads a byte from a buffer
 *
 * Note:            None
 *****************************************************************************/

BYTE ReadByte( BYTE* pBuffer, WORD index )
{
    return( pBuffer[index] );
}


/******************************************************************************
 * Function:        BYTE ReadWord(BYTE * pBuffer, WORD index)
 *
 * PreCondition:    None
 *
 * Input:           pBuffer   - pointer to a buffer to read from
 *               index   - index in the buffer to read to
 *
 * Output:          WORD   - the word read
 *
 * Side Effects:    None
 *
 * Overview:        Reads a 16-bit word from a buffer
 *
 * Note:            None
 *****************************************************************************/

WORD ReadWord( BYTE* pBuffer, WORD index )
{
    BYTE loByte, hiByte;
    WORD res;
    
    loByte = pBuffer[index];
    hiByte = pBuffer[index+1];
    res = hiByte;
    res *= 0x100;
    res |= loByte;
    return( res );
}


/******************************************************************************
 * Function:        BYTE ReadDWord(BYTE * pBuffer, WORD index)
 *
 * PreCondition:    None
 *
 * Input:           pBuffer   - pointer to a buffer to read from
 *               index   - index in the buffer to read to
 *
 * Output:          DWORD   - the double word read
 *
 * Side Effects:    None
 *
 * Overview:        Reads a 32-bit double word from a buffer
 *
 * Note:            None
 *****************************************************************************/

DWORD ReadDWord( BYTE* pBuffer, WORD index )
{
    WORD loWord, hiWord;
    DWORD result;
    
    loWord = ReadWord( pBuffer, index );
    hiWord = ReadWord( pBuffer, index+2 );
    
    result = hiWord;
    result *= 0x10000;
    result |= loWord;
    return result;
}

#ifdef USE_PIC18

/******************************************************************************
 * Function:        void OpenSPIM( unsigned char sync_mode, unsigned char bus_mode, unsigned char smp_phase)
 *
 * PreCondition:    None
 *
 * Input:           sync_mode   - Sets synchronization
 *               bus_mode   - Sets bus operating mode
 *               smp_phase   - Set sampling phase
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Enables SPI module
 *
 * Note:            None
 *****************************************************************************/

void OpenSPIM( unsigned char sync_mode, unsigned char bus_mode, unsigned char smp_phase)
{
    SPISTAT &= 0x3F;               // power on state
    SPICON1 = 0x00;                // power on state
    SPICON1 |= sync_mode;          // select serial mode 
    SPISTAT |= smp_phase;          // select data input sample phase
    
    switch( bus_mode )
    {
        case 0:                       // SPI1 bus mode 0,0
            SPISTATbits.CKE = 1;       // data transmitted on rising edge
            break;    
        case 2:                       // SPI1 bus mode 1,0
            SPISTATbits.CKE = 1;       // data transmitted on falling edge
            SPICON1bits.CKP = 1;       // clock idle state high
            break;
        case 3:                       // SPI1 bus mode 1,1
            SPICON1bits.CKP = 1;       // clock idle state high
            break;
        default:                      // default SPI1 bus mode 0,1
            break;
    }

    switch( sync_mode )
    {
        case 4:                       // slave mode w /SS1 enable
            SD_CS_TRIS = 1;       // define /SS1 pin as input
        case 5:                       // slave mode w/o /SS1 enable
            SPICLOCK = 1;       // define clock pin as input
            SPISTATbits.SMP = 0;       // must be cleared in slave SPI mode
            break;
        default:                      // master mode, define clock pin as output
            SPICLOCK = 0;       // define clock pin as output
            break;
    }

    SPIOUT = 0;                  // define SDO1 as output (master or slave)
    SPIIN = 1;                  // define SDI1 as input (master or slave)
    SPICON1bits.SSPEN = 1;             // enable synchronous serial port 
}


/******************************************************************************
 * Function:        unsigned char WriteSPIM ( unsigned char data_out)
 *
 * PreCondition:    None
 *
 * Input:           data_out   - data to transmit
 *
 * Output:          0 if successful, -1 otherwise
 *
 * Side Effects:    None
 *
 * Overview:        Writes a byte on the SPI
 *
 * Note:            None
 *****************************************************************************/

unsigned char WriteSPIM( unsigned char data_out )
{
    SPIBUF = data_out;          // write byte to SSP1BUF register
    if ( SPICON1 & 0x80 )       // test if write collision occurred
        return ( -1 );              // if WCOL bit is set return negative #
    else
    {
        while( !SPISTAT_RBF ); // wait until bus cycle complete 
    }
    return ( 0 );                // if WCOL bit is not set return non-negative#
}

/******************************************************************************
 * Function:        void WriteSPIM (void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Disables the SPI module
 *
 * Note:            None
 *****************************************************************************/

void CloseSPIM( void )
{
    SPICON1 &= 0xDF;               // disable synchronous serial port
}



#endif

