/*
 *  EEPROM-Programmer - Read and write EEPROM memories.
 *  Copyright (C) 2022  Fernando Coda <fcoda@pm.me>
 *  Created on: 12 jun. 2021
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "main.h"

/*
 * Electrical connections:
 *
 * uC I2C2:
 * 	 PB10: SCL
 * 	 PB11: SDA
 *
 * EEPROM (DIP-8):
 * 	 1.  A0: GND
 * 	 2.  A1: GND
 * 	 3.  A2: VCC
 * 	 4. Vss: GND
 * 	 5. SDA: SDA
 * 	 6. SCL: SCL
 * 	 7.  WP: GND
 * 	 8. Vcc: Vcc
 *
 * Add a pullup resistor on SCL and SDA.
 * For 100kHz as used here, 10kOhm should do.
 * If changed to 400kHz or 1MHz, use a 2kOhm resistor (or similar)
 */

#define SEND(x) do {if((x) != HAL_OK) return HAL_ERROR;} while(0)
#define RECV(x) do {if((x) != HAL_OK) return HAL_ERROR;} while(0)

static int cmdHasData(uint8_t command);

uint8_t        g_buffer[PKG_DATA_MAX];


HAL_StatusTypeDef sendCommand(uint8_t cmd) {
	return sendPackage(cmd, NULL, 0);
}

HAL_StatusTypeDef sendCommandWithData(uint8_t cmd, uint8_t data) {
	return sendPackage(cmd, &data, 1);
}

HAL_StatusTypeDef sendPackage(uint8_t cmd, uint8_t *data, uint16_t len) {

	SEND(serial_writebyte(CMD_STARTXFER));
	SEND(serial_writebyte(cmd));

	if(data != NULL && len != 0) {
		SEND(serial_write(data, len));
	}

	uint8_t crc[2] = {0,0}; // TODO: crc
	SEND(serial_write(crc, 2));

	SEND(serial_writebyte(CMD_ENDXFER));

	return HAL_OK;
}

HAL_StatusTypeDef sendErr(errorcode_t status) {
	return sendPackage(CMD_ERR, (uint8_t*)&status, 1);
}

HAL_StatusTypeDef sendOK(void) {
	return sendCommand(CMD_OK);
}

HAL_StatusTypeDef receivePackage(package_t *pkg) {

	//	<STX><COMMAND>[<DATA><DATA>...]<CHECKSUM[1]><CHECKSUM[0]><ETX>
	
	if(!serial_available())
		return HAL_ERROR;

	uint8_t *buf = g_buffer;
	uint8_t tmp[3];

	if(pkg == NULL)
		return HAL_ERROR;
	
	pkg->cmd = CMD_ERR;
	pkg->data = NULL;

	RECV(serial_read(tmp, 2));

	if(tmp[0] != CMD_STARTXFER)
		return HAL_ERROR;

	pkg->cmd = tmp[1];
	pkg->datalen = cmdHasData(pkg->cmd);

	if(pkg->datalen != 0)
	{
		RECV(serial_read(buf, pkg->datalen));
		pkg->data = buf;
	}

	RECV(serial_read(tmp, 3));
	
	// TODO: { tmp[1], tmp[0] } is the checksum, we can ignore it...
	if(tmp[2] != CMD_ENDXFER)
		return HAL_ERROR;

	return HAL_OK;
}

int cmdHasData(command_t command) {
	switch(command) {

	case CMD_INIT:  return 0;
	case CMD_MEMID: return 1;

	case CMD_READMEM: return 1; /* contains the memtype_e */
	case CMD_READNEXT: return 0;
	case CMD_MEMDATA: return PKG_DATA_MAX;
	case CMD_DATA: return 1;
	case CMD_INFO: return PKG_DATA_MAX;

	case CMD_WRITEMEM: return 1;

	case CMD_OK:  return 0;
	case CMD_ERR: return 1;

	case CMD_NONE:
	case CMD_PING:
	case CMD_TXRX_ACK:
	case CMD_TXRX_DONE:
	case CMD_TXRX_ERR:
	case CMD_STARTXFER:
	case CMD_ENDXFER:
	case CMD_DISCONNECT:
		return 0;
	}
	return 0;
}

static errorcode_t sendMemoryBlock(uint8_t cmd, uint16_t offset)
{
//	package_t pkg;
	uint8_t *buf = g_buffer;

	if(readMemoryBlock(buf, offset) != HAL_OK) {
		return ERROR_READMEM;
	}
	if(sendPackage(cmd, buf, PKG_DATA_MAX) != HAL_OK) {
		return ERROR_COMM;
	}
	return ERROR_NONE;
}

static int sendNext(uint16_t mem_idx, int *st) {
	errorcode_t ret = sendMemoryBlock(CMD_MEMDATA, mem_idx);
	if (ret == ERROR_NONE) {
		*st = CMD_READNEXT;
	}
	else {
		sendErr(ret);
		if (ret == ERROR_COMM)
			*st = 0;
		else
			*st = 1;
	}
	return (int) ret;
}

static bool isCommandValid(int st, command_t cmd)
{
	switch(st)
	{
	case 0:
		if(cmd == CMD_INIT) return true;
		break;
	case 1:
		if(		cmd == CMD_READMEM ||
				cmd == CMD_WRITEMEM ||
				cmd == CMD_PING ||
				cmd == CMD_DISCONNECT ||
				cmd == CMD_MEMID)
			return true;
		break;
	default:
		break;
	}
	return false;
}


/*********************************************************/

void uart_fsm(void)
{
	static int st=0;
	static uint32_t timeout = TIMEOUT_MS;
	static uint16_t retries = 0;
	static package_t package = {0};
	static uint16_t mem_idx = 0;
	HAL_StatusTypeDef ret;

	if(st != 0 && HAL_GetTick() > timeout) {
		st = 0;
	}

	switch (st)
	{
	case 0: /* disconnected */
		led_off();
		// try to establish connection with serial port server
		ret = receivePackage(&package);

		if (ret == HAL_OK) {
			if(package.cmd == CMD_INIT) {
				sendCommand(CMD_INIT);
				led_on();
				st = CMD_MEMID;
				timeout = HAL_GetTick()+TIMEOUT_MS;
			}
		}
		break;

	case CMD_MEMID:
		ret = receivePackage(&package);
		if(ret == HAL_OK && package.cmd == CMD_MEMID)
		{
			enum memtype_e memid = package.data[0];
			if(EEPROM_InitMemory(memid) == HAL_OK) {
				sendCommand(CMD_OK);
				st = 1;
			}
			else {
				sendErr(ERROR_MEMID);
				st = 0;
			}
		}
		break;

	case 1: /* waiting to receive a package */
		ret = receivePackage(&package);
		if(ret != HAL_OK)
			break;

		if(!isCommandValid(st,package.cmd)) {
			st = 0;
			break;
		}

		timeout = HAL_GetTick()+TIMEOUT_MS;
		st = package.cmd;
		break;

	case CMD_READMEM: /* received READMEM */
		if(package.data[0] == g_memtype) {
			mem_idx = 0;
			sendCommand(CMD_OK);
			timeout = HAL_GetTick()+TIMEOUT_MS;
			st = CMD_TXRX_ACK;
		}
		else {
			sendErr(ERROR_MEMID);
			st = 1;
		}
		break;

	case CMD_TXRX_ACK: /* waiting to send data */
		ret = receivePackage(&package);
		if(ret != HAL_OK)
			break;

		if(package.cmd == CMD_READNEXT)
		{
			sendNext(mem_idx, &st);
			retries = 0;
		}
		else {
			sendErr(ERROR_UNKNOWN);
			st = 0;
		}
		break;

	case CMD_READNEXT: /* Chunk of memory sent. Waiting acknowledge */
		ret = receivePackage(&package);
		if(ret != HAL_OK)
			break;

		if(package.cmd == CMD_TXRX_ACK) {
			// go send next chunk
			mem_idx += PKG_DATA_MAX;
			if(mem_idx >= g_memsize) {
				// PC is doing some stupid shit
				sendErr(ERROR_MEMIDX);
				st = 1;
			}
			else {
				st = CMD_TXRX_ACK;
			}
		}
		else if(package.cmd == CMD_TXRX_DONE) {
			st = 1;
		}
		else if(package.cmd == CMD_TXRX_ERR && retries < RETRIES_MAX) {
			// resend current chunk
			sendNext(mem_idx, &st);
			++retries;
		}
		else {
			// something went wrong
			if(retries >= RETRIES_MAX)
				sendErr(ERROR_MAX_RETRY);
		//	sendCommand(CMD_DISCONNECT);
			st = 0;
		}

		timeout = HAL_GetTick()+TIMEOUT_MS;
		break;

	case CMD_WRITEMEM:

		if(package.data[0] == g_memtype) {
			mem_idx = 0;
			timeout = HAL_GetTick()+TIMEOUT_MS;
			st = CMD_MEMDATA;
			sendCommand(CMD_OK);
		}
		else {
			st = 1;
			sendErr(ERROR_MEMID);
		}
		break;

	case CMD_MEMDATA: /* wait to receive memory data */
		ret = receivePackage(&package);
		if(ret != HAL_OK)
			break;

		if(package.cmd == CMD_MEMDATA)
		{
			int status = HAL_OK;
			if((mem_idx + package.datalen) <= g_memsize) {
				// write to eeprom the content received
				status = saveMemoryBlock(package.data, mem_idx);
				if(status == HAL_OK) {
					mem_idx += package.datalen;
					if(mem_idx >= g_memsize) {
						sendCommand(CMD_TXRX_DONE);
						st = 1;
					}
					else {
						sendCommand(CMD_TXRX_ACK);
					}
				}
				else {
					sendErr(ERROR_WRITEMEM);
					st = 1;
				}
			}
			else {
				sendErr(ERROR_MEMIDX);
				st = 0;
			}
		}
		else {
			if(package.cmd != CMD_ERR)
				sendErr(ERROR_UNKNOWN);
			st = 0;
		}

		timeout = HAL_GetTick()+TIMEOUT_MS;
		break;

	case CMD_PING:
		sendCommand(CMD_TXRX_ACK);
		timeout = HAL_GetTick()+TIMEOUT_MS;
		st = 1;
		break;

	case CMD_DISCONNECT:
		st = 0;
		break;

	default:
		st=0;
		break;
	}
}
// TODO: split this...

/* ----------------------------------------------------------------------- */
#if 0

int write_test()
{
	int ret = HAL_OK;
	uint8_t membuffer[0x2000];
	extern struct memory_info memory[];
	uint16_t memsz = memory[g_memtype].size;

	HAL_Delay(1000);
	serial_clearScreen();
	serial_println("About to write entire memory...");
	HAL_Delay(2000);
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, LED_ON);

	for(int i=0; i<memsz; ++i)
		membuffer[i]=(uint8_t)i;
	membuffer[0x11E0]=0x66;
	membuffer[0x1E8F]=0x99;//random test

	ret = EEPROM_write(g_memtype, membuffer, 0, memsz);
/*	for(uint16_t register_address = 0x0000U; register_address < 0x2000U; register_address += 32U)
	{
		for(int i=0; i<32; i++)
			pagebuffer[i] = (uint8_t) register_address+i;

		while( (ret=HAL_I2C_Mem_Write(&hi2c2, mem24lc64_addr, register_address, 2, pagebuffer, 32, 1000)) != HAL_OK );

		sprintf((char*)Buffer, "0x%X-0x%X OK", register_address, register_address+32);
		serial_println((char*)Buffer);
	}*/

	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, LED_OFF);

	char buf[32];
	if(ret == HAL_OK)
		sprintf(buf, "\r\nDone.");
	else
		sprintf(buf, "\r\nERROR %d WRITING MEMORY.", ret);
	serial_println(buf);

	return ret;
}

void print_writeProtectRegister() {
	uint8_t reg = 0;
	char buf[32];
	int ret = EEPROM_read_reg(MEMTYPE_X24645, &reg, 0x1FFF);
	if(ret == HAL_OK) {
		sprintf(buf, "Register WPR is 0x%02X", reg );
		serial_println(buf);
	}
	else {
		serial_println("Can't read register WPR");
	}
}

int read_test()
{
	uint16_t register_address = 0x0000U;
	int ret = HAL_OK;
	char buf[64] = "";
	uint8_t pagebuffer[32] = {0};
	extern struct memory_info memory[];
	uint8_t pagesz = memory[g_memtype].pageSz;

	HAL_Delay(1500);
	serial_clearScreen();
	if(g_memtype == MEMTYPE_X24645) print_writeProtectRegister();
	serial_println("About to read entire memory...\r\n");
	HAL_Delay(2000);
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, LED_ON);


	for(register_address = 0; ret == HAL_OK && register_address < g_memsize; register_address += pagesz)
	{
		ret = EEPROM_read_page(g_memtype, pagebuffer, register_address);

		int i;
		char *p = buf;
		sprintf(p, "%04X: ", register_address );
		p+=6;
		for(i=0; i<16; ++i) {
			sprintf(p+3*i, "%02X ", pagebuffer[i] );
		}
		serial_println(buf);

		p=buf;
		sprintf(p, "%04X: ", register_address+16 );
		p+=6;
		for(i=0; i<16; ++i) {
			sprintf(p+3*i, "%02X ", pagebuffer[16+i] );
		}
		serial_println(buf);
	}

	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, LED_OFF);

	if(ret == HAL_OK)
		sprintf(buf, "\r\nDone.");
	else
		sprintf(buf, "\r\nERROR %d READING MEMORY.", ret);
	serial_println(buf);

	return ret;
}

void i2c_scanner(int startAddress)
{
	char buf[64] = "";
	int dev, ret;
	HAL_Delay(5000);
	serial_println("\r\nStarting I2C scan...");
	HAL_Delay(1000);
	for(dev=startAddress; dev<128; ++dev)
	{
		ret = HAL_I2C_IsDeviceReady(&hi2c2, dev << 1, 3, 5);
		if(ret != HAL_OK) {
			serial_print(" - ");
		}
		else {
			sprintf(buf, " 0x%X ", dev);
			serial_print(buf);
		}
	}

	serial_println("Done!\r\n");
}

#endif


