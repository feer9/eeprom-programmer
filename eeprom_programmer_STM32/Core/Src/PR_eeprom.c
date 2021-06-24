/*
 * PR_eeprom.c
 *
 *  Created on: 12 jun. 2021
 *      Author: feer
 */

#include "main.h"



int readMemory(uint8_t *buffer)
{
	if(g_memtype== MEMTYPE_24LC16) {
		return MEM24LC16_read(buffer, 0, g_memsize);
	}
	else if(g_memtype == MEMTYPE_X24645) {
		return MEMX24645_read(buffer, 0, g_memsize);
	}
	return -1;
}

int saveMemory(const uint8_t *data)
{
	if(g_memtype == MEMTYPE_24LC16)
		return MEM24LC16_write(data, 0, g_memsize);
	else if(g_memtype == MEMTYPE_X24645)
		return MEMX24645_write(data, 0, g_memsize);
	return -1;
}

