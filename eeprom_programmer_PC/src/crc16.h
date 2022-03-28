#ifndef CRC16_H
#define CRC16_H

#include <QByteArray>
#include "eeprom.h"

class CRC16
{
private:
	static QByteArray wordToByteArray(uint16_t data);

public:
	CRC16();

	static uint16_t gen(uint8_t data);
	static uint16_t gen(const QByteArray& data);
	static uint16_t gen(const uint8_t *data, uint32_t len);
	static uint16_t gen(uint8_t data1, const QByteArray& data2);

	static QByteArray genByteArray(uint8_t data);
	static QByteArray genByteArray(const QByteArray& data);
	static QByteArray genByteArray(const uint8_t *data, uint32_t len);
	static QByteArray genByteArray(uint8_t data1, const QByteArray& data2);

	static uint16_t arrayToWord(const QByteArray& crc);

	static bool check(package_t *pkg);
};

#endif // CRC16_H
