/*******************************************************************************
(C) Copyright 2009 Styl Solutions Co., Ltd. , All rights reserved              *
                                                                               *
This source code and any compilation or derivative thereof is the sole         *
property of Styl Solutions Co., Ltd. and is provided pursuant to a             *
Software License Agreement. This code is the proprietary information           *
of Styl Solutions Co., Ltd and is confidential in nature. Its use and          *
dissemination by any party other than Styl Solutions Co., Ltd is               *
strictly limited by the confidential information provisions of the             *
Agreement referenced above.                                                    *
*******************************************************************************/

/**
 * @file    mlsScannerSetup.c
 * @brief   C program help to setup parameter for SSI protocol that will use to communicate with decoder
 *
 * Long description.
 * @date    13/10/2017
 * @author  alvin.nguyen
 */

/********** Include section ***************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <error.h>
#include <errno.h>
#include <glib.h>

#include "mlsBarcode.h"

/********** Local Type definition section *************************************/
/********** Local Constant and compile switch definition section **************/
/********** Local Macro definition section ************************************/
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define STYL_INFO(format, ...)      printf(ANSI_COLOR_BLUE); \
                                    mlsScannerSetup_Print(0, "[STYL SSI SCANNER]: " format "\n", ##__VA_ARGS__); \
                                    printf(ANSI_COLOR_RESET);

#define STYL_ERROR(format, ...)     printf(ANSI_COLOR_RED); \
                                    mlsScannerSetup_Print(1, "[STYL SSI SCANNER]: " format "\n", ##__VA_ARGS__); \
                                    printf(ANSI_COLOR_RESET);

#define HELP_STRING                                                                         \
        "============================================================================="     \
        "\n\tSTYL program - configure for scanner SSI device.\n"                            \
        "\n\tUsage: StylScannerSetup"                                                       \
        "\n\t(Auto-scaning for scanner device and setup auto mode)"                         \
        "\n\tUsage: StylScannerSetup manual|auto"                                           \
        "\n\t(Auto-scaning for scanner device)"                                             \
        "\n\tUsage: StylScannerSetup /dev/ttyxxx manual|auto"                               \
        "\n============================================================================="

/********** Local (static) variable declaration section ***********************/
/********** Local (static) function declaration section ***********************/
static void     mlsScannerSetup_Help        ();
static void mlsScannerSetup_Print(int isError, char *format, ...);

/********** Local (static) function definition section ************************/

static void mlsScannerSetup_Help ()
{
    printf("%s\n%s\n%s\n", ANSI_COLOR_YELLOW, HELP_STRING, ANSI_COLOR_RESET);
}

static void mlsScannerSetup_Print(int isError, char *format, ...)
{
    if (isError == 0)
        if(getenv("STYL_DEBUG")==NULL)
            return;

    va_list args;
    va_start(args, format);
    //printf(ANSI_COLOR_BLUE);
    vprintf(format, args);
    //printf(ANSI_COLOR_RESET);
    va_end(args);
}



/********** Global function declaration section *******************************/
/********** Global function definition section ********************************/

int main(int argc, const char * argv[])
{
    gchar *scannerPort = NULL;
    gint   scannerMode = STYL_SCANNER_NONEMODE;

    switch(argc)
    {
    case 1:
        scannerPort = g_strdup(mlsBarcodeReader_GetDevice());
        scannerMode = STYL_SCANNER_AUTOMODE;
        break;
    case 2:
        scannerPort = g_strdup(mlsBarcodeReader_GetDevice());
        if (g_strcmp0("auto", argv[1])==0)
            scannerMode = STYL_SCANNER_AUTOMODE;
        else if (g_strcmp0("manual", argv[1])==0)
            scannerMode = STYL_SCANNER_MANUALMODE;
        break;
    case 3:
        scannerPort = g_strdup(argv[1]);
        if (g_strcmp0("auto", argv[2])==0)
            scannerMode = STYL_SCANNER_AUTOMODE;
        else if (g_strcmp0("manual", argv[2])==0)
            scannerMode = STYL_SCANNER_MANUALMODE;
        break;
    default:
        mlsScannerSetup_Help();
        return 1;
    }

    STYL_INFO("SSI Scanner Port %s", scannerPort);
    STYL_INFO("SSI Scanner Mode %d", scannerMode);

    if(scannerMode==STYL_SCANNER_NONEMODE | !scannerPort)
    {
        mlsScannerSetup_Help();
        return 1;
    }

    if(mlsBarcodeReader_Setup(scannerPort, scannerMode)==EXIT_FAILURE)
    {
        STYL_ERROR("Configure SSI protocol for scanner fail.");
    }

    g_free(scannerPort);
    return 0;
}

/**@}*/
