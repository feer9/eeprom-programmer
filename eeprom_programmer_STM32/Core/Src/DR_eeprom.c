/*
 * DR_eeprom.c
 *
 *  Created on: 12 jun. 2021
 *      Author: feer
 */

#include "main.h"



struct memory_info
memory[MEMTYPE_mAX] = {
	{0,0,0,0},                 // MEMTYPE_NONE
	{0x50U, 0x0800U, 16U, 1U}, // MEMTYPE_24LC16
	{0x54U, 0x2000U, 32U, 2U}, // MEMTYPE_24LC64
	{0x00U, 0x2000U, 32U, 1U}, // MEMTYPE_X24645
	{    0, 0x8000U,   0,  0}  // MEMTYPE_24LC256 (unknown yet)
//address7; size  ;pageSz;addrSz;
};


// Memory pin 1: GND - pin 2: GND - pin 3: VCC
// 24LC16B answers to address 0x50 to 0x57
// 24LC64 answers to address 0x54
// X24645 answers to address 0x00 to 0x1F


/*************************************************************************************************/

static uint16_t getDevAddress(memtype_t device, uint16_t register_address)
{
	uint16_t DevAddress = memory[device].address7;

	// Set specific device registers
	switch(device) {
	case MEMTYPE_24LC16:
		DevAddress = DevAddress | ((register_address >> 8) & 0x07U);
		break;
	case MEMTYPE_24LC64:
		// This is constant, just the 7bit address
		break;
	case MEMTYPE_X24645:
		DevAddress = DevAddress | ((register_address >> 8) & 0x1FU);
		break;
	default:
		break;
	}

	// Convert to 8bit address
	DevAddress <<= 1;
	return DevAddress;
}

static int write_aux(memtype_t device, const uint8_t *buffer, uint16_t register_address, uint16_t Size)
{
	int ret = HAL_ERROR;
	uint32_t timeout = HAL_GetTick() + 20;
	// 24LC64 memory takes 5 ms to perform a page write cycle;
	// max 32 bytes at 100kHz should take about 3.5 ms

	uint16_t DevAddress = getDevAddress(device, register_address);
	uint16_t MemAddress = register_address;
	uint16_t MemAddSz   = memory[device].addrSz;

	while((ret = HAL_I2C_IsDeviceReady(&hi2c2, DevAddress, 1, 5)) != HAL_OK
			&& HAL_GetTick() < timeout);

	if(ret == HAL_OK) {
		uint32_t timeleft = timeout - HAL_GetTick();
		ret = HAL_I2C_Mem_Write(&hi2c2, DevAddress, MemAddress, MemAddSz,
								(uint8_t *)buffer, Size, timeleft);
	}

	return ret;
}

int EEPROM_write_reg(memtype_t device, uint8_t reg, uint16_t register_address)
{
	return write_aux(device, &reg, register_address, 1);
}

int EEPROM_write_page(memtype_t device, const uint8_t *page, uint16_t register_address)
{
	uint16_t size = memory[device].pageSz;
	return write_aux(device, page, register_address, size);
}

int EEPROM_write(memtype_t device, const uint8_t *buffer, uint16_t register_base, uint16_t size)
{
	int ret = HAL_OK;
	uint16_t register_top = register_base + size;
	uint32_t register_address;
	uint16_t page_size = memory[device].pageSz;

	for(register_address = register_base; register_address < register_top; register_address += page_size)
	{
		int memindex = register_address - register_base;
		if( (ret=EEPROM_write_page(device, &buffer[memindex], register_address)) != HAL_OK)
			break;
	}

	return ret;
}

/*************************************************************************************************/

static int read_aux(uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSz,
					uint8_t *Buf, uint16_t Size, uint32_t Timeout)
{
	int ret = HAL_ERROR;
	uint32_t tstart = HAL_GetTick();

	while((ret = HAL_I2C_IsDeviceReady(&hi2c2, DevAddress, 1, 5)) != HAL_OK
			&& HAL_GetTick()-tstart < Timeout);

	if(ret == HAL_OK) {
		Timeout -= HAL_GetTick()-tstart;
		ret=HAL_I2C_Mem_Read(&hi2c2, DevAddress, MemAddress, MemAddSz, Buf, Size, Timeout);
	}

	return ret;
}

int EEPROM_read_page(memtype_t device, uint8_t *page, uint16_t register_address)
{
	uint16_t DevAddress = getDevAddress(device, register_address);
	uint16_t MemAddress = register_address;
	uint16_t MemAddSz   = memory[device].addrSz;
	uint16_t Size       = memory[device].pageSz;
	uint32_t Timeout    = 20;

	return read_aux(DevAddress, MemAddress, MemAddSz, page, Size, Timeout);
}

int EEPROM_read(memtype_t device, uint8_t *buf, uint16_t register_base, uint16_t size)
{
	uint16_t DevAddress = getDevAddress(device, register_base);
	uint16_t MemAddress = register_base;
	uint16_t MemAddSz   = memory[device].addrSz;
	uint16_t Size       = size;
	uint32_t Timeout    = (uint32_t)(size/memory[device].pageSz)*5 + 10;

	return read_aux(DevAddress, MemAddress, MemAddSz, buf, Size, Timeout);
}

int EEPROM_read_reg(memtype_t device, uint8_t *reg, uint16_t register_address)
{
	uint16_t DevAddress = getDevAddress(device, register_address);
	uint16_t MemAddress = register_address;
	uint16_t MemAddSz   = memory[device].addrSz;
	uint16_t Size       = 1;
	uint32_t Timeout    = 20;

	return read_aux(DevAddress, MemAddress, MemAddSz, reg, Size, Timeout);
}

/*************************************************************************************************/

void MEMX24645_enableWriteAccess()
{
	if(HAL_OK != EEPROM_write_reg(MEMTYPE_X24645, 0x02, 0x1FFF)) {
		while(1); // Do not continue
	}
}

uint32_t getMemSize(enum memtype_e memtype)
{
	return memory[memtype].size;
}

static int query_devices(void)
{
	int status = HAL_ERROR;

	for(int i=1; i<MEMTYPE_mAX; ++i)
	{
		status = HAL_I2C_IsDeviceReady(&hi2c2, memory[i].address7 << 1U, 3U, 5U);
		if(status == HAL_OK) {
			g_memtype = i;
			break;
		}
	}

	return status;
}

void EEPROM_Init()
{
	while(query_devices() != HAL_OK)
		HAL_Delay(10);

	g_memsize = getMemSize(g_memtype);

	if(g_memtype == MEMTYPE_X24645) {
		MEMX24645_enableWriteAccess();
	}
}




/*
 * 	Note from 24LC16 datasheet:
	Page write operations are limited to
	writing bytes within a single physical page,
	regardless of the number of bytes
	actually being written. Physical page
	boundaries start at addresses that are
	integer multiples of the page buffer size
	(or ‘page-size’) and end at addresses that
	are integer multiples of [page size – 1]. If
	a Page Write command attempts to write
	across a physical page boundary, the
	result is that the data wraps around to the
	beginning of the current page (overwriting
	data previously stored there), instead of
	being written to the next page, as might be
	expected. It is therefore necessary for the
	application software to prevent page write
	operations that would attempt to cross a
	page boundary.
 * */


#if 0
/* -------------------- MEM24LC16 -------------------- */
// 24LC16 MEMORY PAGES ARE ONLY 16 BYTES LONG !!!
int MEM24LC16_write_page(const uint8_t *page, uint16_t register_address)
{
	int ret = HAL_OK, attempts = 120; // TODO: tune attempts value
	uint8_t control_byte = (memaddr7[MEMTYPE_24LC16] | ((register_address >> 8) & 0x07U)) << 1U;
	uint8_t word_address = register_address & 0xFFU;


	while(--attempts &&
		  ((ret=HAL_I2C_Mem_Write(&hi2c2, control_byte, word_address, 1, (uint8_t *)page, 16, 1000)) != HAL_OK) );

	return ret;
}

int MEM24LC16_write(const uint8_t *buffer, uint16_t register_base, uint16_t size)
{
	int ret = HAL_OK;
	uint16_t register_top = register_base + size;
	uint32_t register_address;

	for(register_address = register_base; register_address < register_top; register_address += 16)
	{
		if( (ret=MEM24LC16_write_page(&buffer[register_address], register_address)) != HAL_OK)
			break;
	}

	return ret;
}

int MEM24LC16_read_page(uint8_t *page, uint16_t register_address)
{
	int ret = HAL_OK, attempts = 10;
	uint8_t control_byte = (memaddr7[MEMTYPE_24LC16] | ((register_address >> 8) & 0x07U)) << 1U;
	uint8_t word_address = register_address & 0xFFU;

	while(--attempts &&
			((ret=HAL_I2C_Mem_Read(&hi2c2, control_byte, word_address, 1, page, 32, 1000)) != HAL_OK) );

	return ret;
}

int MEM24LC16_read(uint8_t *buf, uint16_t register_base, uint16_t size)
{
	int ret = HAL_OK;
	uint16_t register_top = register_base + size; // ¿-1?
	uint32_t register_address;

	for(register_address = register_base; register_address < register_top; register_address += MEM24LC16_PAGE_SZ)
	{
		if ((ret=MEM24LC16_read_page(&buf[register_address], register_address)) != HAL_OK)
			break;
	}

	return ret;
}
#endif
