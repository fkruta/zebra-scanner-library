//
//  main.c
//  zebra_scanner_C
//
//  Created by Hoàng Trung Huy on 8/22/16.
//  Copyright © 2016 Hoàng Trung Huy. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>

#include "mlsBarcode.h"

#define BUFFER_LEN	4000

int main(int argc, const char * argv[])
{
	int ret = EXIT_SUCCESS;
	int barcodeLen = 0;
	char buff[BUFFER_LEN];
	memset(buff, 0, BUFFER_LEN);

	ret = mlsBarcodeReader_Open();
	if (ret)
	{
		goto EXIT;
	}

	ret = mlsBarcodeReader_ReadData(buff, BUFFER_LEN);
	if (ret <= 0)
	{
		goto EXIT;
	}
	else
	{
		barcodeLen = ret;
		printf("\e[36mBarcode(%d):\n%s\e[0m\n", barcodeLen, buff);
	}

	mlsBarcodeReader_Close();

EXIT:
	return ret;
}
