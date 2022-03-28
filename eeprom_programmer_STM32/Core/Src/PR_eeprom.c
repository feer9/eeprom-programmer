/*
 * PR_eeprom.c
 *
 *  Created on: 12 jun. 2021
 *      Author: feer
 */

#include "main.h"



int readMemoryBlock(uint8_t *buffer, uint16_t offset)
{
	return EEPROM_read(g_memtype, buffer, offset, PKG_DATA_MAX);
}

int readMemory(uint8_t *buffer)
{
	return EEPROM_read(g_memtype, buffer, 0, g_memsize);
}

int saveMemory(const uint8_t *data)
{
	return EEPROM_write(g_memtype, data, 0, g_memsize);
}

int saveMemoryBlock(const uint8_t *data, uint16_t offset)
{
	int status = EEPROM_write(g_memtype, data, offset, PKG_DATA_MAX);
	if(status != HAL_OK)
		return status;
	uint8_t tmpbuf[PKG_DATA_MAX] = {0};
	status = readMemoryBlock(tmpbuf, offset);
	if(status != HAL_OK)
		return status;
	if(memcmp(data, tmpbuf, PKG_DATA_MAX) != 0)
		return ERROR_WRITEMEM;
	return HAL_OK;
}
