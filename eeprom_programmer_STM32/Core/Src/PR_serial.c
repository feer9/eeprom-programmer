/*
 * PR_serial.c
 *
 *  Created on: 12 jun. 2021
 *      Author: feer
 */

#include "main.h"

#include "usbd_cdc_if.h"



int serial_clearScreen(void) {
	uint8_t clearScreen[] = {0x1B, 0x5B, 0x32, 0x4A};
	return serial_write(clearScreen, sizeof clearScreen);
}

#if 0 // UART

int serial_write(const uint8_t *data, uint16_t len) {
	uint8_t retries = 5;
	int ret = 1;

	while(retries-- && ret != HAL_OK) {
		ret = HAL_UART_Transmit(&huart2, (uint8_t *) data, len, 1000) ;
	}

	return ret;
}

int serial_read(uint8_t *data, uint16_t len) {
	return HAL_UART_Receive(&huart2, data, len, 1000) ;
}

#else // USB CDC

static int write(const uint8_t *data, uint16_t sz)
{
	int ret = USBD_FAIL;

	while ( (ret = CDC_Transmit_FS((uint8_t *) data, sz)) == USBD_BUSY );

	return ret;
}

int serial_write(const uint8_t *data, uint16_t len)
{
	uint32_t tstart = HAL_GetTick();
	int ret = USBD_FAIL;

	while(ret != USBD_OK && (HAL_GetTick()-tstart) < 1000)
		ret = write(data, len);

	return ret;
}

static int read(uint8_t *buf, uint16_t sz)
{
	uint16_t bytesAvailable = CDC_GetRxBufferBytesAvailable_FS();
	uint16_t bytesReaded = 0;

	if (bytesAvailable > 0) {

		uint16_t bytesToRead = bytesAvailable >= sz ? sz : bytesAvailable;

		if (CDC_ReadRxBuffer_FS(buf, bytesToRead) == USB_CDC_RX_BUFFER_OK) {
			bytesReaded = bytesToRead;
		}
	}

	return bytesReaded;
}

int serial_read(uint8_t *buffer, uint16_t len)
{
	uint32_t tstart = HAL_GetTick();
	uint16_t bytesRemaining = len;

	while(bytesRemaining > 0 && (HAL_GetTick()-tstart) < 1000)
	{
		uint16_t bytesToRead = bytesRemaining > HL_RX_BUFFER_SIZE ? HL_RX_BUFFER_SIZE : bytesRemaining;

		uint16_t bytesReaded = read(buffer, bytesToRead);

		bytesRemaining -= bytesReaded;
		buffer += bytesReaded;
	}

	if(bytesRemaining != 0)
		return HAL_ERROR;

	return HAL_OK;
}

#endif


#if 0
int serial_print(const char *s) {
	return (int) HAL_UART_Transmit(&huart2, (uint8_t *) s, strlen(s), 100);
}

int serial_println(const char *s) {
	int err = (int) HAL_UART_Transmit(&huart2, (uint8_t *) s, strlen(s), 100);
	if(!err) {
		err = (int) HAL_UART_Transmit(&huart2, (uint8_t *) "\r\n", 2, 100);
	}
	return err;
}

int serial_printnum(const char *s, int num) {
	int err = 0;
	if(s) {
		err = (int) HAL_UART_Transmit(&huart2, (uint8_t *) s, strlen(s), 100);
	}
	if(!err) {
		char buf[32];
		itoa(num, buf, 10);
		int len = (int) strlen(buf);

		err = (int) HAL_UART_Transmit(&huart2, (uint8_t *) buf, len, 100);
	}
	return err;
}

int serial_printnumln(const char *s, int num) {
	int err = 0;
	if(s) {
		err = (int) HAL_UART_Transmit(&huart2, (uint8_t *) s, strlen(s), 100);
	}
	if(!err) {
		char buf[32];
		itoa(num, buf, 10);
		strcat(buf, "\r\n");
		int len = (int) strlen(buf);

		err = (int) HAL_UART_Transmit(&huart2, (uint8_t *) buf, len, 100);
	}
	return err;
}
#endif

#if 0
// doesn't work :(
int _write(int32_t file, uint8_t *ptr, int32_t len) {
	(void)file;
	/* Implement your write code here, this is used by puts and printf for example */
	int i = 0;
	for(i=0; i < len; i++) {
		ITM_SendChar((*ptr++));
	}
	return len;
}
#endif
