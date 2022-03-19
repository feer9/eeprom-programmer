/*
 * AP_application.c
 *
 *  Created on: 12 jun. 2021
 *      Author: feer
 */
#include "main.h"

/*
 * Electrical connections:
 *
 * I2C2:
 * 	 SCL: PB10
 * 	 SDA: PB11
 *
 * EEPROM (DIP-8):
 * 	 1: GND
 * 	 2: GND
 * 	 3: VCC
 * 	 4: GND
 * 	 5: SDA
 * 	 6: SCL
 * 	 7: GND
 * 	 8: VCC
 */


#define SEND(x) do {if((x) != HAL_OK) return HAL_ERROR;} while(0)
#define RECV(x) do {if((x) != HAL_OK) return HAL_ERROR;} while(0)


static int cmdHasData(uint8_t command);

enum memtype_e g_memtype = MEMTYPE_NONE;
uint32_t g_memsize;

//uint8_t membuffer[PACKAGE_SZ];
uint8_t g_buffer[PKG_DATA_MAX];




HAL_StatusTypeDef sendCommand(uint8_t cmd) {
	return sendPackage(cmd, NULL, 0);
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

HAL_StatusTypeDef sendErr(uint8_t status) {
	return sendPackage(CMD_ERR, &status, 1);
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

int cmdHasData(commands_t command) {
	switch(command) {

	case CMD_INIT:  return 0;
	case CMD_MEMID: return 1;

	case CMD_READMEM: return 1; /* contains the memtype_e */
	case CMD_READNEXT: return 0;
	case CMD_MEMDATA: return PKG_DATA_MAX;

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

static void sendNext(uint16_t mem_idx, int *st) {
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

	if(HAL_GetTick() > timeout) {
		if(st != 0)
			sendErr(ERROR_TIMEOUT);
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
				st++;
				timeout = HAL_GetTick()+TIMEOUT_MS;
			}
		}
		break;

	case 1: /* waiting to receive a package */
		ret = receivePackage(&package);
		if(ret != HAL_OK)
			break;

		timeout = HAL_GetTick()+TIMEOUT_MS;
		st = package.cmd;
		break;

	case CMD_READMEM:
		if(package.data[0] != g_memtype) {
			sendErr(ERROR_MEMID);
			st = 1;
			break;
		}
		mem_idx = 0;
		timeout = HAL_GetTick()+TIMEOUT_MS;
		// set state so next time we skip memtype verification
		st = CMD_TXRX_ACK;
		// FALLTHROUGH

	case CMD_TXRX_ACK:
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
			st = CMD_TXRX_ACK;
		}
		else if(package.cmd == CMD_TXRX_ERR && retries < RETRIES_MAX) {
			// resend current chunk
			sendNext(mem_idx, &st);
			++retries;
		}
		else if(package.cmd == CMD_TXRX_DONE) {
			st = 1;
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

/*
	case CMD_WRITEMEM:
		requested_mem = st & 0x0F;
		if(requested_mem != g_memtype) {
			sendErr(2);
			st = 1;
		}

		// write to eeprom the content received
		saveMemory(package.data);

		st = 1;
		break;
*/

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






/*HAL_StatusTypeDef sendMemory()
{
	uint8_t cmd = CMD_READMEM|g_memtype;
	uint32_t offset;

	for(offset=0; offset<g_memsize; offset+=MAX_PKG_SZ)
	{
		int retry = 5, status = HAL_ERROR;
		while(retry-- && status != HAL_OK) {
			status = trySend(cmd, offset);
		}
		if(status != HAL_OK) {
			// they didn't receive it
			return HAL_ERROR;
		}
	}

	return HAL_OK;
}*/

/*
HAL_StatusTypeDef sendMemory(uint8_t cmd)
{
	int offset, retry;
	package_t pkg;

	for(offset=0; offset<g_memsize; offset+=MAX_PKG_SZ) {

		retry = 5;
		while(retry--) {

			if(readMemoryBlock(g_buffer, offset) != HAL_OK) {
				sendErr(10);
				return HAL_ERROR;
			}
			if(sendBlock(cmd, g_buffer) != HAL_OK) {
				sendErr(11);
				return HAL_ERROR;
			}
			if(receivePackage(&pkg) != HAL_OK) {
				return HAL_ERROR;
			}
			if(pkg.cmd == CMD_TXRX_OK) {
				// block successfully received
				break; // while()
			}
		}
		if(pkg.cmd != CMD_TXRX_OK) { // didn't receive it
			return HAL_ERROR;
		}
	}

	return HAL_OK;
}
 **/


/* ----------------------------------------------------------------------- */
#if 1

int write_test()
{
	int ret = HAL_OK;
	uint8_t membuffer[0x2000];
	extern struct memory_info memory[];
	uint8_t memsz = memory[g_memtype].size;

	HAL_Delay(5000);
	serial_clearScreen();
	serial_println("About to write entire memory...");
	HAL_Delay(8000);
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, LED_ON);

	for(int i=0; i<memsz; ++i)
		membuffer[i]=(uint8_t)i;

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

	if(ret == HAL_OK)
		serial_println("\r\nDone.");
	else
		serial_println("\r\nERROR WRITING MEMORY.");

	return ret;
}

int read_test()
{
	uint16_t register_address = 0x0000U;
	int ret = HAL_OK;
	char buf[64] = "";
	uint8_t pagebuffer[32] = {0};
	extern struct memory_info memory[];
	uint8_t pagesz = memory[g_memtype].pageSz;

	HAL_Delay(5000);
	serial_clearScreen();
	serial_println("About to read entire memory...\r\n");
	HAL_Delay(8000);
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
		serial_println("\r\nDone.");
	else
		serial_println("\r\nERROR READING MEMORY.");

	return ret;
}

void i2c_scanner(int startAddress)
{
	char buf[64] = "";
	int dev, ret;
	HAL_Delay(5000);
	serial_println("Starting I2C scan...");
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

	serial_println("Done!");
}

#endif


