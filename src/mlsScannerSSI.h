/*******************************************************************************
 *  (C) Copyright 2009 STYL Solutions Co., Ltd. , All rights reserved          *
 *                                                                             *
 *  This source code and any compilation or derivative thereof is the sole     *
 *  property of STYL Solutions Co., Ltd. and is provided pursuant to a         *
 *  Software License Agreement.  This code is the proprietary information      *
 *  of STYL Solutions Co., Ltd and is confidential in nature.  Its use and     *
 *  dissemination by any party other than STYL Solutions Co., Ltd is           *
 *  strictly limited by the confidential information provisions of the         *
 *  Agreement referenced above.                                                *
 ******************************************************************************/

/**
 * @file    mlsScannerSSI.h
 * @brief   C code - Implement SSI Communication.
 *
 * Long description.
 * @date    22/09/2017
 * @author  Alvin Nguyen - alvin.nguyen@styl.solutions
 */

#ifndef MLSSCANNERSSI_H_INCLUDED
#define MLSSCANNERSSI_H_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

/********** Include section ****************************************************/
#include "glib.h"

/********** Constant  and compile switch definition section ********************/
/********** Type declaration section *******************************************/
/********** Macro declaration section ******************************************/
/********** Function declaration section ***************************************/

/*!
 * \brief mlsScannerSSI_Read: read formatted package and response ACK from/to scanner via file descriptor
 * \return number of read bytes
 */
gint mlsScannerSSI_Read(gint pFile, byte *buffer, gint sizeBuffer, const gint timeout_ms);

/*!
 * \brief mlsScannerSSI_Write: write formatted package and check ACK to/from scanner via file descriptor
 * \return
 * - EXIT_SUCCESS: Success
 * - EXIT_FAILURE: Fail
 */
gint mlsScannerSSI_Write(gint pFile, byte opcode, byte *param, byte paramLen, gboolean sendWakeup, gboolean isPermanent);

/*!
 * \brief mlsScannerSSI_CheckACK: receive ACK package after mlsScannerSSI_Write() and check for ACK
 * \return
 * - EXIT_SUCCESS: Success	ACK
 * - EXIT_FAILURE: Fail		Unknown cause
 */
gint mlsScannerSSI_CheckACK(gint pFile);

/*!
 * \brief mlsScannerSSI_IsContinue: check package's type
 * \return
 * - 0  : Is last package in multiple packages stream
 * - 1 : Is intermediate package in multiple packages stream
 */
gint mlsScannerSSI_IsContinue(byte *package);

/*!
 * \brief mlsScannerSSI_CorrectPackage: check package's is correct
 * \return
 * - 1 : Receive package is correct
 * - 0 : Receive package is incorrect
 */
gint mlsScannerSSI_CorrectPackage(byte *package);


/*!
 * \brief mlsScannerSSI_SendCommand: Send a command then check ACK
 * \return
 * - EXIT_SUCCESS: Success
 * - EXIT_FAILURE: Fail
 */
gint mlsScannerSSI_SendCommand(gint pFile, byte opCode);

/*!
 * \brief mlsScannerSSI_GetACK: get ACK raw package
 * \return number of read bytes
 */
gint mlsScannerSSI_GetACK(gint pFile, byte *buffer, gint sizeBuffer, const gint timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // MLSSCANNERSSI_H_INCLUDED
/**@}*/
