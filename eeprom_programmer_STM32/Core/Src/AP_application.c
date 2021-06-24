/*
 * AP_application.c
 *
 *  Created on: 12 jun. 2021
 *      Author: feer
 */

#include "main.h"


static const uint8_t cmd_startxfer[] = {0xFF, 0x55, 0xAA};
static const uint8_t cmd_endxfer[]   = {0xAA, 0x55, 0xFF};
#define cmd_init		0x01
#define cmd_ping		0x02
#define cmd_disconnect  0xf1

/* read eeprom and send to PC */
#define cmd_readmem16	(0x40 | MEMTYPE_24LC16)
#define cmd_readmemx64	(0x40 | MEMTYPE_X24645)
#define cmd_readmem256	(0x40 | MEMTYPE_24LC256)
/* get data from pc and write to eeprom */
#define cmd_writemem16	(0x50 | MEMTYPE_24LC16)
#define cmd_writememx64	(0x50 | MEMTYPE_X24645)
#define cmd_writemem256	(0x50 | MEMTYPE_24LC256)

#define cmd_ok			0x10
#define cmd_err			0xFF

/*
 * PACKAGE STRUCTURE:
 * <STX><COMMAND>[<DATA><DATA>...]<ETX>
 */


#define SEND(x) do {if((x) != HAL_OK) return HAL_ERROR;} while(0)
#define RECV(x) do {if((x) != HAL_OK) return HAL_ERROR;} while(0)

typedef struct {
	uint8_t cmd;
	uint16_t datalen;
	uint8_t *data;
} package_t;

static int cmdHasData(uint8_t command);
static int check_start(uint8_t *p);
static int check_end(uint8_t *p);
int sendPackage(uint8_t cmd, uint8_t *data, uint16_t len);
HAL_StatusTypeDef receivePackage(package_t *pkg);

enum memtype_e g_memtype = MEMTYPE_NONE;
uint32_t g_memsize;

uint8_t membuffer[0x2000];
uint8_t recvbuffer[128];











int sendCommandStart() {
	return serial_write(cmd_startxfer, 3);
}
int sendCommandEnd() {
	return serial_write(cmd_endxfer, 3);
}

int sendCommand(uint8_t cmd) {
	return sendPackage(cmd, NULL, 0);
}

int sendPackage(uint8_t cmd, uint8_t *data, uint16_t len) {

	SEND(sendCommandStart());
	SEND(serial_write(&cmd, 1));

	if(data != NULL && len != 0) {
		SEND(serial_write(data, len));
	}

	SEND(sendCommandEnd());

	return 0;
}

int sendErr(uint8_t status) {
	return sendPackage(cmd_err, &status, 1);
}

int sendOK(void) {
	return sendCommand(cmd_ok);
}

HAL_StatusTypeDef receivePackage(package_t *pkg) {
	uint8_t *buf = recvbuffer;

	if(pkg == NULL)
		return HAL_ERROR;

	RECV(serial_read(buf, 4));

	if(check_start(buf) != 0)
		return HAL_ERROR;

	pkg->cmd = buf[3];
	pkg->datalen = cmdHasData(pkg->cmd);
	pkg->data = NULL;

	if(pkg->datalen != 0) {

		if((pkg->cmd & 0xF0) == 0x50) // if receiving whole memory
			buf = membuffer;

		RECV(serial_read(buf, pkg->datalen));
		pkg->data = buf;
	}

	uint8_t buf2[3];
	RECV(serial_read(buf2, 3));
	if(check_end(buf2) != 0)
		return HAL_ERROR;

	return HAL_OK;
}

int cmdHasData(uint8_t command) {
	switch(command) {

	case cmd_init:

	case cmd_readmem16:
	case cmd_readmemx64:
	case cmd_readmem256: return 0;

	case cmd_writemem16: return getMemSize(MEMTYPE_24LC16);
	case cmd_writememx64: return getMemSize(MEMTYPE_X24645);
	case cmd_writemem256: return getMemSize(MEMTYPE_24LC256); // will have to split this

	case cmd_ok:  return 0;
	case cmd_err: return 1;

	default: return 0;
	}
}

int check_start(uint8_t *p) {
	return memcmp(p, cmd_startxfer, 3);
}

int check_end(uint8_t *p) {
	return memcmp(p, cmd_endxfer, 3);
}

int sendMemory()
{
	return serial_write(membuffer, g_memsize);
}

int receiveMemory()
{
	return serial_read(membuffer, g_memsize);
}



/*********************************************************/

void uart_fsm(void)
{
	static int st = 0, requested_mem;
	static uint32_t retries = 0;
	static package_t package;
	HAL_StatusTypeDef ret;

	switch (st)
	{
	case 0:
		led_off();
		// try to establish connection with serial port server
		ret = receivePackage(&package);

		if (ret == HAL_OK) {
			if(package.cmd == cmd_init) {
				sendCommand(cmd_init);
				led_on();
				st++;
			}
		}
		break;
	case 1:

		ret = receivePackage(&package);
		if(ret != HAL_OK) {
			if(++retries >= 5) {
				retries = 0;
				st = 0;
			}
			break;
		}
		retries = 0;
		st = package.cmd;

		int tmp = st & 0xF0;
		if(tmp == 0x40 || tmp == 0x50) { // if requesting memory w/r

			requested_mem = st & 0x0F;
			if(requested_mem != g_memtype) {
				sendErr(2);
				st = 1;
			}
		}

		break;

	case cmd_readmem16:  // this isn't a state, unless I had used interruptions
	case cmd_readmemx64:
		// read memory and send back the content
		if(readMemory(membuffer) == HAL_OK) {
			sendPackage(package.cmd, membuffer, g_memsize);
		}
		else {
			sendErr(10);
		}
		st = 1;
		break;

	case cmd_readmem256:
		st = 1;
		break;

	case cmd_writemem16:
	case cmd_writememx64:
		// write to eeprom the content received
		if(saveMemory(package.data) != HAL_OK) {
			sendErr(st);
		}
		else {
			sendOK();
		}
		st = 1;
		break;

	case cmd_writemem256:
		st = 1;
		break;


	case cmd_ping:
		sendPackage(cmd_ping, NULL,0);
		st = 1;
		break;

	case cmd_disconnect:
		st = 0;
		break;

	default:
		st=0;
		break;
	}
}



/* ----------------------------------------------------------------------- */
#if 0

int write_test()
{
//	uint16_t register_address = 0x0000;
	int ret = HAL_OK;

	serial_clearScreen();
	serial_println("About to write entire memory...");
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);

	for(int i=0; i<0x2000; ++i)
		membuffer[i]=i;

	ret = MEMX24645_write(membuffer, 0, 0x2000);
/*	for(register_address = 0; register_address < 0x2000; register_address += 32)
	{
		for(int i=0; i<32; i++)
			pagebuffer[i] = (uint8_t) register_address+i;

		while( (ret=HAL_I2C_Mem_Write(&hi2c2, mem24lc64_addr, register_address, 2, pagebuffer, 32, 1000)) != HAL_OK );

		sprintf((char*)Buffer, "0x%X-0x%X OK", register_address, register_address+32);
		serial_println((char*)Buffer);
	}*/

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);

	if(ret == HAL_OK)
		serial_println("\r\nDone.");
	else
		serial_println("\r\nERROR WRITING MEMORY.");

	return ret;
}

int read_test()
{
	uint16_t register_address = 0x0000;
	int ret = HAL_OK;
	char buf[64] = "";
	uint8_t pagebuffer[PAGE_SZ] = {0};

	memset(pagebuffer, 0x00, 32U);

	serial_clearScreen();
	serial_println("About to read entire memory...\r\n");
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);


	for(register_address = 0; ret == HAL_OK && register_address < g_memsize; register_address += PAGE_SZ)
	{
		ret = MEMX24645_read_page(pagebuffer, register_address);

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

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
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
	serial_println("Starting I2C scan...");
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


