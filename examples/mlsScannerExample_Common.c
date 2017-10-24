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
 * @file    mlsScannerExample_Common.c
 * @brief   C code - define some utils for example applications
 *
 * Long description.
 * @date    22/09/2017
 * @author  Alvin Nguyen - alvin.nguyen@styl.solutions
 */

/********** Include section ***************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "mlsScannerExample_Common.h"

/********** Include section ***************************************************/
/********** Local Constant and compile switch definition section **************/
/********** Local Type definition section *************************************/
/********** Local Macro definition section ************************************/
/********** Local (static) variable definition ********************************/
/********** Local (static) function declaration section ***********************/
/********** Local function definition section *********************************/
/********** Global function definition section ********************************/

/*!
 * \brief mlsScannerExample_Print: Print out some information.
 */
void mlsScannerExample_Print(int isError, char *format, ...)
{
    if (isError == 0)
        if(getenv("STYL_DEBUG")==NULL)
            return;

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

/*@}*/
