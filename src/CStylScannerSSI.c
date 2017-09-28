/*******************************************************************************
 *  (C) Copyright 2009 STYL Solutions Co., Ltd. , All rights reserved       *
 *                                                                             *
 *  This source code and any compilation or derivative thereof is the sole     *
 *  property of STYL Solutions Co., Ltd. and is provided pursuant to a      *
 *  Software License Agreement.  This code is the proprietary information      *
 *  of STYL Solutions Co., Ltd and is confidential in nature.  Its use and  *
 *  dissemination by any party other than STYL Solutions Co., Ltd is        *
 *  strictly limited by the confidential information provisions of the         *
 *  Agreement referenced above.                                                *
 ******************************************************************************/

/**
 * @file    CStylScannerSSI.h
 * @brief   C code - Implement SSI Communication.
 *
 * Long description.
 * @date    22/09/2017
 * @author  Alvin Nguyen - alvin.nguyen@styl.solutions
 */

/********** Include section ***************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <error.h>
#include <errno.h>

#include "CStylScannerUtils.h"
#include "CStylScannerSSI.h"
#include "CStylScannerPackage.h"

/********** Local Type definition section *************************************/
/********** Local Constant and compile switch definition section **************/
/********** Local Macro definition section ************************************/
/********** Local (static) variable declaration section ***********************/
/********** Local (static) function declaration section ***********************/
static void     StylScannerSSI_PreparePackage           (byte *package, byte opcode, byte *param, byte paramLen);
static uint16_t StylScannerSSI_CalculateChecksum        (byte *package, gint length);
static gint     StylScannerSSI_IsChecksumOK             (byte *package);
static gint     StylScannerSSI_SerialRead               (gint pFile, byte* buffer, guint sizeBuffer, guint timeout_ms);
static gint     StylScannerSSI_SerialWrite              (gint pFile, byte* buffer, guint sizeBuffer, guint timeout_ms);
/********** Local (static) function definition section ************************/

/*!
 * \brief StylScannerSSI_SerialRead: read data from serial port
 * \return
 * - number of readed bytes or O if no byte was readed
 */
static gint StylScannerSSI_SerialRead(gint pFile, byte* buffer, guint sizeBuffer, guint timeout_ms)
{
    /*  Initialize the file descriptor set.*/
    fd_set  fds;
    FD_ZERO (&fds);
    FD_SET  (pFile, &fds);

    /* Initialize the timeout. */
    struct timeval tv;
    tv.tv_sec  = (timeout_ms + sizeBuffer) / 1000;
    tv.tv_usec = ((timeout_ms % 1000) +sizeBuffer) * 1000;


    gint nbytes = 0;
    while (nbytes < sizeBuffer)
    {
        /*See if there is data available. */
        gint rc = select (pFile + 1, &fds, NULL, NULL, &tv);
        /* TODO: Recalculate timeout here! */
        if (rc < 0)
        {
            /* ********* Error during select call **************** */
            STYL_ERROR("select: %d - %s", errno, strerror(errno));
            return 0;
        }
        else if (rc == 0)
        {
            /* ********* Timeout ********************************* */
            break;
        }

        /* ********* Read the available data. ******************** */
        gint n = read (pFile, buffer + nbytes, sizeBuffer - nbytes);
        if (n < 0)
        {
            /* ********* Error during select call **************** */
            STYL_ERROR("read: %d - %s", errno, strerror(errno));
            return 0;
        }
        else if (n == 0)
            break; /* EOF reached. */

        /* Increase the number of bytes read. */
        nbytes += n;
    }
    /* ********* Return the number of bytes read. ***************** */
    return nbytes;
}

/*!
 * \brief StylScannerSSI_SerialWrite: write data to serial port
 * \return
 * - number of readed bytes or O if no byte was readed
 */
 //unsigned int PM_UART::sendBuff(unsigned char* buff, unsigned int len, unsigned int timeout_ms)
static gint StylScannerSSI_SerialWrite(gint pFile, byte* buffer, guint sizeBuffer, guint timeout_ms)
{
    guint   sizeSent    = 0;
    guint   n           = 0;
    struct  timeval     tv ;
    fd_set  sset;

    FD_ZERO(&sset);
    FD_SET(pFile, &sset);

    tv.tv_sec = (timeout_ms + sizeBuffer) / 1000;
    tv.tv_usec =((timeout_ms % 1000) + sizeBuffer) * 1000;

    while (sizeSent < sizeBuffer)
    {
        switch (select(pFile + 1, NULL, &sset, NULL, &tv))
        {
        case -1:
            STYL_ERROR("select: %d - %s", errno, strerror(errno));
            return 0;
        case 0:
            STYL_ERROR("select: %d - %s", errno, strerror(errno));
            return -1;
        default:
            if (FD_ISSET(pFile, &sset) != 0)
            {
                n = write(pFile, buffer + sizeSent, sizeBuffer - sizeSent);
                if (n <= 0)
                {
                    STYL_ERROR("write: %d - %s", errno, strerror(errno));
                    return 0;
                }
            }
            else
            {
                return 0;
            }

            break;
        }

        sizeSent += n;
    }

    STYL_INFO("Serial sent %d bytes", sizeSent);

    return sizeSent;
}

/*!
 * \brief StylScannerSSI_IsChecksumOK: check 2 last bytes for checksum
 * \return
 * - TRUE : checksum is correct
 * - FALSE: checksum is incorrect
 */
static gint StylScannerSSI_IsChecksumOK(byte *package)
{
    uint16_t checksum = 0;

    checksum += package[PACKAGE_LEN(package) + 1];
    checksum += package[PACKAGE_LEN(package)] << 8;
    STYL_INFO("checksum receive: %x", checksum);
    STYL_INFO("checksum calculate: %x", StylScannerSSI_CalculateChecksum(package, PACKAGE_LEN(package)));
    return (checksum == StylScannerSSI_CalculateChecksum(package, PACKAGE_LEN(package)));
}

/*!
 * \brief StylScannerSSI_IsContinue: check package's type
 * \return
 * - 0  : Is last package in multiple packages stream
 * - 1 : Is intermediate package in multiple packages stream
 */
gint StylScannerSSI_IsContinue(byte *package)
{
    return (SSI_PARAM_STATUS_CONTINUATION & package[PKG_INDEX_STAT]);
}

/*!
 * \brief StylScannerSSI_CorrectPackage: check package's is correct
 * \return
 * - 1 : Receive package is correct
 * - 0 : Receive package is incorrect
 */
gint StylScannerSSI_CorrectPackage(byte *package)
{
    return !(SSI_ID_DECODER | package[PKG_INDEX_SRC]);
}


/*!
 * \brief PreparePkg: generate package from input opcode and params
 * \param
 */
static void StylScannerSSI_PreparePackage(byte *package, byte opcode, byte *param, byte paramLen)
{
    uint16_t checksum = 0;
    /*
        #define PKG_INDEX_LEN						0
        #define PKG_INDEX_OPCODE					1
        #define PKG_INDEX_SRC						2
        #define PKG_INDEX_STAT						3
    */
    package[PKG_INDEX_LEN]      = SSI_LEN_HEADER;
    package[PKG_INDEX_OPCODE]   = opcode        ;
    package[PKG_INDEX_SRC]      = SSI_ID_HOST   ;

    if(opcode == SSI_CMD_PARAM          ||
       opcode == SSI_CMD_SCAN_DISABLE   ||
       opcode == SSI_CMD_SCAN_ENABLE     )
        package[PKG_INDEX_STAT]     = SSI_PARAM_TYPE_PERMANENT;
    else
        package[PKG_INDEX_STAT]     = SSI_PARAM_TYPE_TEMPORARY; // fix here again

    if ( (NULL != param) && (0 != paramLen) )
    {
        package[PKG_INDEX_LEN] += paramLen;
        memcpy(&package[PKG_INDEX_STAT + 1], param, paramLen);
    }

    /* ***************** Add checksum ************************* */
    checksum = StylScannerSSI_CalculateChecksum(package, PACKAGE_LEN(package));
    package[PACKAGE_LEN(package)]     = checksum >> 8;
    package[PACKAGE_LEN(package) + 1] = checksum & 0xFF;
}

/*!
 * \brief StylScannerSSI_CalculateChecksum: calculate 2's complement of package
 * \param
 * - package: pointer of package content.
 * - length: Length of package in byte.
 * \return 16 bits checksum (no 2'complement) value
 */
static uint16_t StylScannerSSI_CalculateChecksum(byte *package, gint length)
{
    uint16_t checksum = 0;
    for (guint i = 0; i < length; i++)
    {
        checksum += package[i];
    }
    /* ***************** 1's complement *********************** */
    checksum ^= 0xFFFF;	// flip all 16 bits
    /* ***************** 2's complement *********************** */
    checksum += 1;
    return checksum;
}

/********** Global function definition section ********************************/

/*!
 * \brief StylScannerSSI_Read: read formatted package and response ACK from/to scanner via file descriptor
 * \return number of read bytes
 */
gint StylScannerSSI_Read(gint pFile, byte *buffer, gint sizeBuffer, const gint timeout)
{
    gint retValue = 0;
    gint sizeReceived = 0;
    gint readRequest;
    byte recvBuff[PACKAGE_LEN_MAXIMUM];
    gint lastIndex = 0;

    memset(&recvBuff, 0, PACKAGE_LEN_MAXIMUM);

    do
    {
        /* Read 1 first byte for length */
        readRequest = 1;
        retValue += StylScannerSSI_SerialRead(pFile, &recvBuff[PKG_INDEX_LEN], readRequest, TTY_TIMEOUT);
        STYL_INFO("retValue: %d", retValue);
        if(retValue <= 0)
        {
            break;
        }
        else
        {
            STYL_INFO("Length is: %x", recvBuff[PKG_INDEX_LEN]);
            if(recvBuff[PKG_INDEX_LEN] >= PACKAGE_LEN_MAXIMUM)
            {
                STYL_ERROR("Receive value of length package is invalid.");
                retValue = 0;
                break;
            }
            /* Read rest of byte of package */
            STYL_ERROR("recvBuff[PKG_INDEX_LEN]: %x", recvBuff[PKG_INDEX_LEN]);
            STYL_ERROR("SSI_LEN_CHECKSUM: %x", SSI_LEN_CHECKSUM);
            readRequest = recvBuff[PKG_INDEX_LEN] + SSI_LEN_CHECKSUM - 1; /* 1 is byte read before */
            STYL_INFO("Rest byte is: %d", readRequest);

            sizeReceived = StylScannerSSI_SerialRead(pFile, &recvBuff[PKG_INDEX_LEN + 1], readRequest, TTY_TIMEOUT);
            STYL_INFO("");
            StylScannerPackage_Display(recvBuff, NO_GIVEN);
            STYL_INFO("sizeReceived: %d", sizeReceived);
            if(sizeReceived != readRequest)
            {
                retValue = 0;
                goto __error;
            }
            else
            {
                retValue += sizeReceived;
                STYL_INFO("retValue: %d", retValue);

                if(!StylScannerSSI_CorrectPackage(recvBuff))
                {
                    STYL_ERROR("********** Receive a invalid package");
                    retValue = 0;
                    goto __error;
                }

                if (StylScannerSSI_IsChecksumOK(recvBuff))
                {
                    /* Send ACK for buffer received */
                    STYL_INFO("Checksum OK! Send ACK for checksum.");
                    if(StylScannerSSI_Write(pFile, SSI_CMD_ACK, NULL, 0) != EXIT_SUCCESS)
                    {
                        STYL_ERROR("Send ACK for checksum fail.");
                    }
                    else
                    {
                        if(sizeBuffer <= (lastIndex + PACKAGE_LEN(recvBuff) + SSI_LEN_CHECKSUM))
                        {
                            STYL_ERROR("Receive buffer too large.");
                            retValue = 0;
                            break;
                        }
                        /* Copy current buffer to finally buffer */
                        gint sizeMemcpy = PACKAGE_LEN(recvBuff) + SSI_LEN_CHECKSUM;
                        memcpy(&buffer[lastIndex], recvBuff, sizeMemcpy);
                        lastIndex += sizeMemcpy;
                    }
                }
                else
                {
                    STYL_ERROR("*********** Checksum fail!");
                    retValue = 0;
                    goto __error;
                }
            }
        }
    }
    while(StylScannerSSI_IsContinue(recvBuff));

    return retValue;

__error:
    STYL_INFO("Error! Send NAK to scanner");
    if(StylScannerSSI_Write(pFile, SSI_CMD_NAK, NULL, 0) != EXIT_SUCCESS)
    {
        STYL_ERROR("Send NAK to scanner got problem.");
    }
    return retValue;
}

/*!
 * \brief StylScannerSSI_GetACK: get ACK raw package
 * \return number of read bytes
 */
gint StylScannerSSI_GetACK(gint pFile, byte *buffer, gint sizeBuffer, const gint timeout)
{
    gint retValue = 0;
    gint sizeReceived = 0;
    gint readRequest;
    byte recvBuff[PACKAGE_LEN_MAXIMUM];
    gint lastIndex = 0;

    memset(&recvBuff, 0, PACKAGE_LEN_MAXIMUM);

    /* Read 1 first byte for length */
    readRequest = 1;
    retValue = StylScannerSSI_SerialRead(pFile, &recvBuff[PKG_INDEX_LEN], readRequest, TTY_TIMEOUT);
    STYL_INFO("Fisrt byte size: %d", retValue);
    if(retValue <= 0)
    {
        goto __error;
    }
    else
    {
        STYL_INFO("Length is: %x", recvBuff[PKG_INDEX_LEN]);
        if(recvBuff[PKG_INDEX_LEN] >= PACKAGE_LEN_MAXIMUM)
        {
            STYL_ERROR("Receive value of length package is invalid.");
            goto __error;
        }
        /* Read rest of byte of package */
        readRequest = recvBuff[PKG_INDEX_LEN] + SSI_LEN_CHECKSUM - 1; /* 1 is byte read before */
        STYL_INFO("Rest byte is: %d", readRequest);

        sizeReceived = StylScannerSSI_SerialRead(pFile, &recvBuff[PKG_INDEX_LEN + 1], readRequest, TTY_TIMEOUT);
        STYL_INFO("");
        StylScannerPackage_Display(recvBuff, NO_GIVEN);
        STYL_INFO("sizeReceived: %d", sizeReceived);

        if(sizeReceived != readRequest)
        {
            goto __error;
        }
        else
        {
            retValue += sizeReceived;
            STYL_INFO("Total size received: %d", retValue);

            if(!StylScannerSSI_CorrectPackage(recvBuff))
            {
                STYL_ERROR("********** Receive a invalid package");
                goto __error;
            }

            if (StylScannerSSI_IsChecksumOK(recvBuff))
            {
                /* Copy current buffer to finally buffer */
                STYL_DEBUG("MEMCPY SIZE: %d", PACKAGE_LEN(recvBuff) + SSI_LEN_CHECKSUM);
                memcpy(buffer, recvBuff, PACKAGE_LEN(recvBuff) + SSI_LEN_CHECKSUM);
            }
            else
            {
                STYL_ERROR("*********** Checksum fail!");
                goto __error;
            }
        }
    }

    STYL_INFO("ACK buffer size: %d", retValue);
    StylScannerPackage_Display(buffer, NO_GIVEN);
    return retValue;

__error:
    retValue = 0;
    return retValue;

}

/*!
 * \brief StylScannerSSI_Write: write formatted package and check ACK to/from scanner via file descriptor
 * \return
 * - EXIT_SUCCESS: Success
 * - EXIT_FAILURE: Fail
 */
gint StylScannerSSI_Write(gint pFile, byte opcode, byte *param, byte paramLen)
{
    gint retValue = EXIT_SUCCESS;
    gint sizeSend = 0;

    gint  bufferSize    = (SSI_LEN_HEADER + SSI_LEN_CHECKSUM + paramLen) * sizeof(byte);
    byte *bufferContent = malloc(bufferSize);

    /* ***************** Flush old input queue *************** */
    tcflush(pFile, TCIFLUSH);

    StylScannerSSI_PreparePackage(bufferContent, opcode, param, paramLen);

    STYL_WARNING("bufferSize: %d", bufferSize);
    STYL_WARNING("Send data: ");
    StylScannerPackage_Display(bufferContent, bufferSize);

    STYL_WARNING("PACKAGE_LEN(bufferContent) + SSI_LEN_CHECKSUM: %d", PACKAGE_LEN(bufferContent) + SSI_LEN_CHECKSUM);
    sizeSend = PACKAGE_LEN(bufferContent) + SSI_LEN_CHECKSUM;
    if(StylScannerSSI_SerialWrite(pFile, bufferContent, sizeSend, TTY_TIMEOUT) != sizeSend)
    {
        STYL_ERROR("Send data to scanner got problem.");
        retValue = EXIT_FAILURE;
    }
    free(bufferContent);

    return retValue;
}

/*!
 * \brief StylScannerSSI_CheckACK: receive ACK package after StylScannerSSI_Write() and check for ACK
 * \return
 * - EXIT_SUCCESS: Success	ACK
 * - EXIT_FAILURE: Fail		Unknown cause
 */
gint StylScannerSSI_CheckACK(gint pFile)
{
    gint retValue = EXIT_SUCCESS;
    byte recvBuff[PACKAGE_LEN_ACK_MAXIMUM];

    memset(recvBuff, 0, PACKAGE_LEN_ACK_MAXIMUM);
    for (gint i = 0; i < PACKAGE_LEN_ACK_MAXIMUM; i++)
    {
        STYL_INFO_OTHER(" 0x%02x", recvBuff[i]);
    }

    STYL_INFO("Invoke StylScannerSSI_Read");
    retValue = StylScannerSSI_GetACK(pFile, recvBuff, PACKAGE_LEN_ACK_MAXIMUM, 1);
    if ( (retValue > 0) && (SSI_CMD_ACK == recvBuff[PKG_INDEX_OPCODE]) )
    {
        retValue = EXIT_SUCCESS;
    }
    else
    {
        retValue = EXIT_FAILURE;
    }

    return retValue;
}


/*!
 * \brief StylScannerSSI_SendCommand: Send a command then check ACK
 * \return
 * - EXIT_SUCCESS: Success
 * - EXIT_FAILURE: Fail
 */
gint StylScannerSSI_SendCommand(gint pFile, byte opCode)
{
    gint retValue = EXIT_SUCCESS;
    retValue = StylScannerSSI_Write(pFile, opCode, NULL, 0);
    if(retValue==EXIT_SUCCESS)
        retValue = StylScannerSSI_CheckACK(pFile);

    return retValue;
}

/**@}*/

