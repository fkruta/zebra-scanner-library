//
//  ssi_utils.c
//  zebra_scanner_C
//
//  Created by Hoàng Trung Huy on 8/22/16.
//  Copyright © 2016 Hoàng Trung Huy. All rights reserved.
//

#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <sys/signal.h>
#include <sys/time.h>
#include <sys/types.h>

#include "ssi_utils.h"
#include "ssi.h"

#define MSB_16(x)		(x >> 8)
#define LSB_16(x)		(x & UINT8_MAX)

#define TRUE				1
#define FALSE				0


#define TIMEOUT_MSEC		50

static uint16_t CalculateChecksum(byte *pkg);
static void PreparePkg(byte *pkg, byte opcode, byte *param, byte paramLen);
static int IsChecksumOK(byte *pkg);
static int IsContinue(byte *pkg);
static char *strNAK(int code);


/*!
 * \brief strNAK generate NAK message based on code
 * \return NAK message string
 */
static char *strNAK(int code)
{
	char *msg = NULL;

	switch (code) {
		case NAK_RESEND:
			msg = "RESEND";
			break;

		case NAK_CANCEL:
			msg = "CANCEL";
			break;

		case NAK_DENIED:
			msg = "DENIED";
			break;

		case NAK_BAD_CONTEXT:
			msg = "NAK_BAD_CONTEXT";
			break;

	  default:
			break;
	}

	return msg;
}

/*!
 * \brief CalculateChecksum calculate 2's complement of pkg
 * \return 16 bits checksum (2' complement) value
 */
static uint16_t CalculateChecksum(byte *pkg)
{
	int checksum = 0;

	for (unsigned int i = 0; i < PKG_LEN(pkg); i++)
	{
		checksum += pkg[i];
	}

	// 1's complement
	checksum ^= 0xFFFF;	// flip all 16 bits

	// 2's complement
	checksum += 1;

	return checksum;
}

/*!
 * \brief PreparePkg generate package from input opcode and params
 */
static void PreparePkg(byte *pkg, byte opcode, byte *param, byte paramLen)
{
	uint16_t checksum = 0;

	pkg[INDEX_LEN] = SSI_HEADER_LEN;
	pkg[INDEX_OPCODE] = opcode;
	pkg[INDEX_SRC] = SSI_HOST;
	pkg[INDEX_STAT] = SSI_DEFAULT_STATUS;

	if ( (NULL != param) && (0 != paramLen) )
	{
		pkg[INDEX_LEN] += paramLen;
		memcpy(&pkg[INDEX_STAT + 1], param, paramLen);
	}

	// add checksum
	checksum = CalculateChecksum(pkg);
	pkg[PKG_LEN(pkg)] = checksum >> 8;
	pkg[PKG_LEN(pkg) + 1] = checksum & 0xFF;
}

/*!
 * \brief IsChecksumOK check 2 last bytes for checksum
 * \return
 * - TRUE: checksum is correct
 * - FALSE: checksum is incorrect
 */
static int IsChecksumOK(byte *pkg)
{
	uint16_t cksum = 0;

	cksum += pkg[PKG_LEN(pkg) + 1];
	cksum += pkg[PKG_LEN(pkg)] << 8;

	return (cksum == CalculateChecksum(pkg));
}

/*!
 * \brief IsLastPackage check package's type
 * \return
 * - TRUE: Is last package in multiple packages stream
 * - FALSE: Is intermediate package in multiple packages stream
 */
static int IsContinue(byte *pkg)
{
	return (STAT_CONTINUATION & pkg[INDEX_STAT]);
}

/*!
 * \brief DisplayPkg print out package.
 * If PKG_LEN(pkg) > MAX_PKG_LEN, only first MAX_PKG_LEN bytes are displayed
 */
void DisplayPkg(byte *pkg)
{
	char *debug = NULL;
	debug = getenv("SSI_DEBUG");

	if ( (NULL != debug) && (NULL != pkg) )
	{
		for (int i = 0; i < PKG_LEN(pkg); i++)
		{
			printf("0x%x ", pkg[i]);
		}
		printf("\n");
	}
}

/*!
 * \brief ConfigSSI send parameters to configure scanner
 * \return
 * - EXIT_SUCCESS: Success
 * - EXIT_FAILURE: Fail
 */
int ConfigSSI(int fd)
{
	int ret = EXIT_SUCCESS;
	byte param[12] =	{
						PARAM_BEEP_NONE,
						PARAM_B_DEC_FORMAT	, ENABLE,
						PARAM_B_SW_ACK		, ENABLE,
						PARAM_B_SCAN_PARAM	, DISABLE,	// Disable to avoid accidental changes param from scanning
						PARAM_TRIGGER_MODE	, PARAM_TRIGGER_PRESENT,
		PARAM_INDEX_F0,	PARAM_B_DEC_EVENT	, ENABLE
		};

	printf("Configure SSI parameters...");
	ret = WriteSSI(fd, SSI_PARAM_SEND, param, ( sizeof(param) / sizeof(*param) ) );

	if (ret)
	{
		printf("ERROR: %s\n", __func__);
		goto EXIT;
	}
	else
	{
		printf("OK\n");
	}

EXIT:
	return ret;
}

/*!
 * \brief WriteSSI write formatted package and check ACK to/from scanner via file descriptor
 * \return
 * - EXIT_SUCCESS: Success
 * - EXIT_FAILURE: Fail
 */
int WriteSSI(int fd, byte opcode, byte *param, byte paramLen)
{
	int ret = EXIT_SUCCESS;
	byte *sendBuff = malloc( (SSI_HEADER_LEN + paramLen) * sizeof(byte) );
	byte recvBuff[MAX_PKG_LEN];

	// Flush old input queue
	tcflush(fd, TCIFLUSH);

	PreparePkg(sendBuff, opcode, param, paramLen);
	if ( write(fd, sendBuff, PKG_LEN(sendBuff) + 2) <= 0)
	{
		perror("write");
		ret = EXIT_FAILURE;
		goto EXIT;
	}

	// Check ACK
	if (SSI_CMD_ACK != opcode)
	{
		if (ReadSSI(fd, recvBuff, 1) <= 0)
		{
			printf("%s: no ACK\n", __func__);
			ret = EXIT_FAILURE;
		}
		else if (SSI_CMD_NAK == recvBuff[INDEX_OPCODE])
		{
			printf("%s: NAK: %s\n", __func__, strNAK(recvBuff[INDEX_OPCODE]));
		}
		else if (SSI_CMD_ACK != recvBuff[INDEX_OPCODE])
		{
			printf("%s: ACK failure\n", __func__);
			ret = EXIT_FAILURE;
		}
	}

EXIT:
	free(sendBuff);
	return ret;
}

/*!
 * \brief ReadSSI read formatted package and response ACK from/to scanner via file descriptor
 * \return number of read bytes
 */
int ReadSSI(int fd, byte *buff, const int timeout)
{
	int ret = 0;
	int lastIndex = 0;
	int oldVTIME = 0;
	struct termios devConf;
	byte recvBuff[MAX_PKG_LEN];

	tcgetattr(fd, &devConf);
	oldVTIME = devConf.c_cc[VTIME];
	devConf.c_cc[VTIME] = timeout;
	tcsetattr(fd, TCSANOW, &devConf);

	do
	{
		ret += (int) read(fd, &recvBuff[INDEX_LEN]		, 1);// read first byte for length
		if (ret <= 0)
		{
			goto EXIT;
		}
		ret += (int) read(fd, &recvBuff[INDEX_LEN + 1]	, recvBuff[INDEX_LEN] + 1);
		DisplayPkg(recvBuff);
		if ( (ret > 0) || (IsChecksumOK(recvBuff)) )
		{
			WriteSSI(fd, SSI_CMD_ACK, NULL, 0);
			memcpy(&buff[lastIndex], recvBuff, PKG_LEN(recvBuff) + 2);
			lastIndex += PKG_LEN(recvBuff) + 2;
		}
		else
		{
			printf("%s: ERROR", __func__);
			ret = -1;
			goto EXIT;
		}
	} while (IsContinue(recvBuff));

EXIT:
	devConf.c_cc[VTIME] = oldVTIME;
	tcsetattr(fd, TCSANOW, &devConf);
	return ret;
}

/*!
 * \brief WriteSSI write formatted package to scanner via file descriptor
 * \return file descriptor
 */
int OpenTTY(void)
{
	int fd = 0;
	char *devName = getenv("ZEBRA_SCANNER");
//	char *devName = "/dev/tty.usbmodem1421";

	fd = open(devName, O_RDWR, O_NONBLOCK);
	if (fd < 0)
	{
		perror(__func__);
	}

	return fd;
}

/*!
 * \brief WriteSSI write formatted package to scanner via file descriptor
 * \return
 * - EXIT_SUCCESS: Success
 * - EXIT_FAILURE: Fail
 */
int ConfigTTY(int fd)
{
	int ret = EXIT_SUCCESS;
	int flags = 0;
	struct termios devConf;

	// Set flags for blocking mode and sync for writing
	flags = fcntl(fd, F_GETFL);
	if (0 > flags)
	{
		perror("F_GETFL");
		ret = EXIT_FAILURE;
		goto EXIT;
	}
	flags |= (O_FSYNC);
	flags &= ~(O_NDELAY | O_ASYNC);

	ret = fcntl(fd, F_SETFL, flags);
	if (ret)
	{
		perror("F_SETFL");
		ret = EXIT_FAILURE;
		goto EXIT;
	}

	// Configure tty dev
	devConf.c_cflag = (CRTSCTS | CS8 | CLOCAL | CREAD);
	devConf.c_iflag = 0;
	devConf.c_oflag = 0;
	devConf.c_lflag = 0;
	devConf.c_cc[VMIN] = 0;
	devConf.c_cc[VTIME] = TIMEOUT_MSEC;	// read non-blocking flush dump data. See blocking read timeout later

	// Portability: Use cfsetspeed instead of CBAUD since c_cflag/CBAUD is not in POSIX
	ret = cfsetspeed(&devConf, BAUDRATE);
	if (ret)
	{
		perror("Set speed");
		ret = EXIT_FAILURE;
		goto EXIT;
	}

	ret = tcsetattr(fd, TCSANOW, &devConf);
	if (ret)
	{
		perror("Set attribute");
		ret = EXIT_FAILURE;
		goto EXIT;
	}

EXIT:
	return ret;
}

int ExtractBarcode(byte *pkg, char *buff, const int buffLength)
{
	char *barcodePtr = buff;
	byte *pkgPtr = pkg;
	int barcodeLength = 0;
	int barcodePartLength = 0;

	printf("\n");

	if (NULL != pkgPtr)
	{
		while (IsContinue(pkgPtr))
		{
			barcodePartLength = PKG_LEN(pkgPtr) - SSI_HEADER_LEN - 1;
			memcpy(barcodePtr, &pkgPtr[INDEX_BARCODETYPE + 1], barcodePartLength);
			barcodeLength += barcodePartLength;	// 1 byte of barcode type

			// Point to next pkg
			barcodePtr += PKG_LEN(pkgPtr) + 2;
			pkgPtr += PKG_LEN(pkgPtr) + 2;
		}

		// Get the last part of barcode
		barcodePartLength = PKG_LEN(pkgPtr) - SSI_HEADER_LEN - 1;
		memcpy(barcodePtr, &pkgPtr[INDEX_BARCODETYPE + 1], barcodePartLength);
		barcodeLength += barcodePartLength;	// 1 byte of barcode type
	}

	return barcodeLength;
}