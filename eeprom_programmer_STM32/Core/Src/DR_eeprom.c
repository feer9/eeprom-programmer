/*
 * DR_eeprom.c
 *
 *  Created on: 12 jun. 2021
 *      Author: feer
 */

#include "main.h"


//static uint8_t mem24lc64_addr  = 0x54U<<1U; // 8bit address
static uint8_t mem24lc16_addr7 = 0x50U;     // 7bit address
static uint8_t memx24645_addr7 = 0x00U;     // 7bit address




// Memory pin 1: GND - pin 2: GND - pin 3: VCC
// 24LC16B answers to address 0x50 to 0x57
// X24645 answers to address 0x00 to 0x1F


int MEMX24645_write_page(const uint8_t *page, uint16_t register_address)
{
	int ret = HAL_OK, attempts = 100; // just to be sure... it fails with 10
	uint8_t slave_address = (memx24645_addr7 | ((register_address >> 8) & 0x1FU)) << 1U;
	uint8_t byte_address = register_address & 0xFFU;

	while(--attempts &&
		  (ret=HAL_I2C_Mem_Write(&hi2c2, slave_address, byte_address, 1, (uint8_t *)page, 32, 5000)) != HAL_OK);

	return ret;
}

int MEMX24645_write(const uint8_t *buffer, uint16_t register_base, uint16_t size)
{
	int ret = HAL_OK;
	uint16_t register_top = register_base + size;
	uint32_t register_address;

	for(register_address = register_base; register_address < register_top; register_address += PAGE_SZ)
	{
		if( (ret=MEMX24645_write_page(&buffer[register_address], register_address)) != HAL_OK)
			break;
	}

	return ret;
}

int MEMX24645_write_reg(uint8_t reg, uint16_t register_address)
{
	int ret = HAL_OK, attempts = 10;
	uint8_t slave_address = (memx24645_addr7 | ((register_address >> 8) & 0x1FU)) << 1U;
	uint8_t byte_address = register_address & 0xFFU;

	while(--attempts &&
		  (ret=HAL_I2C_Mem_Write(&hi2c2, slave_address, byte_address, 1, &reg, 1, 100)) != HAL_OK);

	return ret;
}

int MEMX24645_read_reg(uint8_t *reg, uint16_t register_address)
{
	int ret = HAL_OK, attempts = 10;
	uint8_t slave_address = (memx24645_addr7 | ((register_address >> 8) & 0x1FU)) << 1U;
	uint8_t byte_address = register_address & 0xFFU;

	while(--attempts &&
		  (ret=HAL_I2C_Mem_Read(&hi2c2, slave_address, byte_address, 1, reg, 1, 100)) != HAL_OK );

	return ret;
}

int MEMX24645_read_page(uint8_t *page, uint16_t register_address)
{
	int ret = HAL_OK, attempts = 10;
	uint8_t slave_address = (memx24645_addr7 | ((register_address >> 8) & 0x1FU)) << 1U;
	uint8_t byte_address = register_address & 0xFFU;

	while(--attempts &&
		  ((ret=HAL_I2C_Mem_Read(&hi2c2, slave_address, byte_address, 1, page, 32, 1000)) != HAL_OK) );

	return ret;
}

int MEMX24645_read(uint8_t *buf, uint16_t register_base, uint16_t size)
{
	int ret = HAL_OK;
	uint16_t register_top = register_base + size;
	uint32_t register_address;

	for(register_address = register_base; register_address < register_top; register_address += PAGE_SZ)
	{
		if ((ret=MEMX24645_read_page(&buf[register_address], register_address)) != HAL_OK)
			break;
	}

	return ret;
}

#if 0
// 24LC64 memory must be written in max blocks of 32 bytes length
int MEM24LC64_write_page(const uint8_t *page, uint16_t register_address)
{
	int ret = HAL_OK, attempts = 10;

	while(--attempts &&
		  (ret=HAL_I2C_Mem_Write(&hi2c2, mem24lc64_addr, register_address, 2, (uint8_t *)page, 32, 1000)) != HAL_OK );

	return ret;
}

int MEM24LC64_write(const uint8_t *buffer, uint16_t register_base, uint16_t size)
{
	int ret = HAL_OK;
	uint16_t register_top = register_base + size;
	uint32_t register_address;

	for(register_address = register_base; register_address < register_top; register_address += PAGE_SZ)
	{
		if( (ret=MEM24LC64_write_page(&buffer[register_address], register_address)) != HAL_OK)
			break;
	}

	return ret;
}

int MEM24LC64_read_page(uint8_t *pagebuffer, uint16_t register_address)
{
	int ret = HAL_OK, attempts = 10;

	while(--attempts &&
		  (ret=HAL_I2C_Mem_Read(&hi2c2, mem24lc64_addr, register_address, 2, pagebuffer, 32, 1000)) != HAL_OK );

	return ret;
}

int MEM24LC64_read(uint8_t *buf, uint16_t register_base, uint16_t size)
{
	int ret = HAL_OK;
	uint16_t register_top = register_base + size;
	uint32_t register_address;

	for(register_address = register_base; register_address < register_top; register_address += PAGE_SZ)
	{
		if ((ret=MEM24LC64_read_page(&buf[register_address], register_address)) != HAL_OK)
			break;
	}

	return ret;
}
#endif

// 24LC16 MEMORY PAGES ARE ONLY 16 BYTES LONG !!!
int MEM24LC16_write_page(const uint8_t *page, uint16_t register_address)
{
	int ret = HAL_OK, attempts = 120; // TODO: tune attempts value
	uint8_t control_byte = (mem24lc16_addr7 | ((register_address >> 8) & 0x07U)) << 1U;
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
	uint8_t control_byte = (mem24lc16_addr7 | ((register_address >> 8) & 0x07U)) << 1U;
	uint8_t word_address = register_address & 0xFF;

	while(--attempts &&
			((ret=HAL_I2C_Mem_Read(&hi2c2, control_byte, word_address, 1, page, 32, 1000)) != HAL_OK) );

	return ret;
}

int MEM24LC16_read(uint8_t *buf, uint16_t register_base, uint16_t size)
{
	int ret = HAL_OK;
	uint16_t register_top = register_base + size; // ¿-1?
	uint32_t register_address;

	for(register_address = register_base; register_address < register_top; register_address += PAGE_SZ)
	{
		if ((ret=MEM24LC16_read_page(&buf[register_address], register_address)) != HAL_OK)
			break;
	}

	return ret;
}

void MEMX24645_enableWriteAccess()
{
	if(HAL_OK != MEMX24645_write_reg(0x02, 0x1FFF)) {
		while(1); // Do not continue
	}
}


uint32_t getMemSize(int memtype) {
	//                           MEMTYPE_24LC16, MEMTYPE_X24645, MEMTYPE_24LC256
	const uint32_t memsizes[] = { 0x800,          0x2000,         0x8000 };

	if(memtype >= MEMTYPE_24LC16 && memtype <= MEMTYPE_mAX)
		return memsizes[memtype];

	return 0;
}


void EEPROM_Init()
{
	int ret = 1;


	while(ret != HAL_OK)
	{
		HAL_Delay(10);
		ret = HAL_I2C_IsDeviceReady(&hi2c2, memx24645_addr7 << 1U, 3u, 5u);
		if(ret == HAL_OK) {
			g_memtype = MEMTYPE_X24645;
			break;
		}
		ret = HAL_I2C_IsDeviceReady(&hi2c2, mem24lc16_addr7 << 1U, 3u, 5u);
		if(ret == HAL_OK) {
			g_memtype = MEMTYPE_24LC16;
			break;
		}
	}
	g_memsize = getMemSize(g_memtype);

	if(g_memtype == MEMTYPE_X24645) {
		MEMX24645_enableWriteAccess();
	}
}






