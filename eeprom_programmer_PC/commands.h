#ifndef COMMANDS_H
#define COMMANDS_H

#include <inttypes.h>

enum modes_e { MODE_NONE, MODE_TX, MODE_RX };

enum memtype_e { MEMTYPE_24LC16, MEMTYPE_X24645, MEMTYPE_24LC256,
				 MEMTYPE_NONE, MEMTYPE_mAX = MEMTYPE_NONE};


static constexpr uint8_t cmd_startxfer[] = {0xFF, 0x55, 0xAA};
static constexpr uint8_t cmd_endxfer[]   = {0xAA, 0x55, 0xFF};
static constexpr uint8_t cmd_init        = 0x01;
static constexpr uint8_t cmd_ping        = 0x02;
static constexpr uint8_t cmd_disconnect  = 0xf1;

/* request uC to read eeprom and sent back the content */
static constexpr uint8_t cmd_read        = (0x40);
static constexpr uint8_t cmd_readmem16   = (cmd_read | MEMTYPE_24LC16);
static constexpr uint8_t cmd_readmemx64  = (cmd_read | MEMTYPE_X24645);
static constexpr uint8_t cmd_readmem256  = (cmd_read | MEMTYPE_24LC256);

/* send data to uC to be written in eeprom */
static constexpr uint8_t cmd_write       = (0x50);
static constexpr uint8_t cmd_writemem16  = (cmd_write | MEMTYPE_24LC16);
static constexpr uint8_t cmd_writememx64 = (cmd_write | MEMTYPE_X24645);
static constexpr uint8_t cmd_writemem256 = (cmd_write | MEMTYPE_24LC256);

static constexpr uint8_t cmd_ok          = 0x10;
static constexpr uint8_t cmd_err         = 0xFF;

#if 1
enum errcodes_e {
	CMD_INIT        = (0x01),
	CMD_PING        = (0x02),
	CMD_DISCONNECT  = (0xF1),

	/* request uC to read eeprom and sent back the content */
	CMD_READ        = (0x40),
	CMD_READMEM16   = (0x40 | MEMTYPE_24LC16),
	CMD_READMEMX64  = (0x40 | MEMTYPE_X24645),
	CMD_READMEM256  = (0x40 | MEMTYPE_X24645),

	/* send data to uC to be written in eeprom */
	CMD_WRITE       = (0x50),
	CMD_WRITEMEM16  = (0x50 | MEMTYPE_24LC16),
	CMD_WRITEMEMX64 = (0x50 | MEMTYPE_X24645),
	CMD_WRITEMEM256 = (0x50 | MEMTYPE_X24645),

	CMD_OK          = (0x10),
	CMD_ERR         = (0xFF)
};
#endif

/*
 * PACKAGE STRUCTURE:
 * <STX[0]><STX[1]><STX[2]><COMMAND>[<DATA><DATA>...]<ETX[0]><ETX[1]><ETX[2]>
 *
 * TODO: <CHKSUM>
 */

#define PKG_MINSIZE int(sizeof cmd_startxfer + sizeof cmd_init + sizeof cmd_endxfer)


typedef struct {
	uint8_t cmd;
	uint16_t datalen;
	uint8_t *data;
} package_t;

#endif // COMMANDS_H
