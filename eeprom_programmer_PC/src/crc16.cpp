#include "crc16.h"

CRC16::CRC16()
{

}

uint16_t CRC16::gen(uint8_t data)
{
	return 0;
}

uint16_t CRC16::gen(uint8_t data1, const QByteArray& data2)
{
	return 0;
}

QByteArray CRC16::wordToByteArray(uint16_t data)
{
	char tmp[2] = {char(data >> 8), char(data & 0xFF)};
	return QByteArray(tmp, 2);
}

QByteArray CRC16::genByteArray(uint8_t data)
{
	uint16_t crc = gen(data);
	return wordToByteArray(crc);
}

QByteArray CRC16::genByteArray(uint8_t data1, const QByteArray& data2)
{
	uint16_t crc = gen(data1, data2);
	return wordToByteArray(crc);
}

uint16_t CRC16::arrayToWord(const QByteArray& array)
{
	uint16_t word = (array[0] << 8) | array[1];
	return word;
}

bool CRC16::check(package_t *pkg)
{
	// TODO: CRC
	return true;
}
