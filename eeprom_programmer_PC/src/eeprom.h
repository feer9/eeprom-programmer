#ifndef EEPROM_H
#define EEPROM_H

#include <QByteArray>
#include <QString>

enum memtype_e {
	MEMTYPE_NONE,
	MEMTYPE_24LC16,
	MEMTYPE_24LC64,
	MEMTYPE_X24645,
	MEMTYPE_24LC256,
	MEMTYPE_mAX
};

enum commands_e: uint8_t {
	CMD_NONE			= 0x00,

	CMD_INIT			= 0x01,
	CMD_PING			= 0x02,
	CMD_MEMID			= 0x03,
	CMD_STARTXFER		= 0xA5, /* not really a command */
	CMD_ENDXFER			= 0x5A, /* not really a command */
	CMD_DISCONNECT		= 0x0F,

	CMD_OK				= 0x10,
	CMD_TXRX_ACK		= 0x11,
	CMD_TXRX_DONE       = 0x12,
	CMD_ERR				= 0xF0, /* general error message followed by an error code */
	CMD_TXRX_ERR		= 0xF1, /* mid transfer error, meant to resend content     */

	/* read eeprom and send to PC */
	CMD_READMEM			= 0x60, /* Specifies which memory is required, and start TX process */
	CMD_READNEXT		= 0x61, /* Request to send next block */

	CMD_MEMDATA         = 0x70, /* Data is being sent over */

	/* get data from pc and write to eeprom */
	CMD_WRITEMEM		= 0x80
};
// TODO: make commands objects of a command class

/*
 * PACKAGE STRUCTURE:
 * <STX><COMMAND>[<DATA><DATA>...]<CHECKSUM[1]><CHECKSUM[0]><ETX>
 */

#define PKG_MINSIZE 5
#define PKG_DATA_MAX 256


struct package_t {
	commands_e cmd;
	uint8_t _[3];
	uint16_t datalen;
	uint16_t crc;
	uint8_t *data;
} ;

struct pkgdata_t {
	commands_e cmd;
	QByteArray *data;
};

class EEPROM
{
public:
	EEPROM();

	static void setMem_24LC16(void);
	static void setMem_X24645(void);
	static void setMem_24LC256(void);
	static bool setTargetMem(memtype_e memtype);
	static bool setTargetMem(QString memtype);

	static memtype_e getMemType(void) {return m_memtype;}
	static qint64 getMemSize(void);
	static qint64 getMemSize(memtype_e);

	static int cmdHasData(commands_e command);

protected:

	static memtype_e m_memtype;
	static int m_memsize;
};

#endif // EEPROM_H
