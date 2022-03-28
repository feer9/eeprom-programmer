/*
 * DR_eeprom.c
 *
 *  Created on: 12 jun. 2021
 *      Author: feer
 */

#include "main.h"



struct memory_info
memory[MEMTYPE_mAX] = {
// address7; size  ;pageSz;addrSz;
	{    0,       0,   0,  0}, // MEMTYPE_NONE
	{0x50U, 0x0800U, 16U, 1U}, // MEMTYPE_24LC16
	{0x54U, 0x2000U, 32U, 2U}, // MEMTYPE_24LC64
	{0x00U, 0x2000U, 32U, 1U}, // MEMTYPE_X24645
	{0x54U, 0x8000U, 64U, 2U}  // MEMTYPE_24LC256
};
enum memtype_e g_memtype = MEMTYPE_NONE;
uint16_t       g_memsize = 0U;


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
	case MEMTYPE_24LC256:
		// This is constant, just the 7bit address
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
	// with a max page size of 64 bytes at 100kHz should take about 6.7 ms

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

int EEPROM_writeReg(memtype_t device, uint8_t reg, uint16_t register_address)
{
	return write_aux(device, &reg, register_address, 1);
}

int EEPROM_writePage(memtype_t device, const uint8_t *page, uint16_t register_address)
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
		if( (ret=EEPROM_writePage(device, &buffer[memindex], register_address)) != HAL_OK)
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

int EEPROM_readPage(memtype_t device, uint8_t *page, uint16_t register_address)
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

int EEPROM_readReg(memtype_t device, uint8_t *reg, uint16_t register_address)
{
	uint16_t DevAddress = getDevAddress(device, register_address);
	uint16_t MemAddress = register_address;
	uint16_t MemAddSz   = memory[device].addrSz;
	uint16_t Size       = 1;
	uint32_t Timeout    = 20;

	return read_aux(DevAddress, MemAddress, MemAddSz, reg, Size, Timeout);
}

/*************************************************************************************************/

static HAL_StatusTypeDef MEMX24645_enableWriteAccess(void)
{
	return EEPROM_writeReg(MEMTYPE_X24645, 0x02, 0x1FFF);
}

uint16_t EEPROM_getMemSize(enum memtype_e memtype)
{
	return memory[memtype].size;
}

static HAL_StatusTypeDef verify_device(enum memtype_e dev_id)
{
	uint16_t addr = memory[dev_id].address7 << 1;
	return HAL_I2C_IsDeviceReady(&hi2c2, addr, 1000U, 50U);
}

HAL_StatusTypeDef EEPROM_InitMemory(enum memtype_e dev_id)
{
	HAL_StatusTypeDef status = verify_device(dev_id);
	if(status == HAL_OK)
	{
		if(dev_id == MEMTYPE_X24645)
			status = MEMX24645_enableWriteAccess();
		if(status == HAL_OK) {
			g_memtype = dev_id;
			g_memsize = EEPROM_getMemSize(dev_id);
		}
	}
	return status;
}
/*static HAL_StatusTypeDef query_devices(void)
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

	g_memsize = EEPROM_getMemSize(g_memtype);

	if(g_memtype == MEMTYPE_X24645) {
		MEMX24645_enableWriteAccess();
	}
}*/

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
