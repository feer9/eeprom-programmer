#include "eeprom.h"

EEPROM::EEPROM()
{
}

memtype_e EEPROM::m_memtype = MEMTYPE_NONE;
int EEPROM::m_memsize = 0;



int EEPROM::cmdHasData(commands_e command) {
	switch(command) {

	case CMD_INIT:  return 0;
	case CMD_MEMID: return 1;

	case CMD_READMEM: return 1;
	case CMD_READNEXT: return 0;
	case CMD_MEMDATA: return PKG_DATA_MAX;
	case CMD_DATA: return 1;
	case CMD_INFO: return PKG_DATA_MAX;

	case CMD_WRITEMEM: return 0;

	case CMD_OK:  return 0;
	case CMD_ERR: return 1;

	case CMD_NONE:
	case CMD_PING:
	case CMD_IDLE:
	case CMD_TXRX_ERR:
	case CMD_TXRX_ACK:
	case CMD_TXRX_DONE:
	case CMD_STARTXFER:
	case CMD_ENDXFER:
	case CMD_DISCONNECT:
		return 0;
	}
	return 0;
}

QString EEPROM::getErrorMsg(errorcode_e err) {
	switch(err)
	{
	case ERROR_NONE:      return QString("No error");
	case ERROR_UNKNOWN:   return QString("Unknown error");
	case ERROR_MEMID:     return QString("Invalid memory ID");
	case ERROR_READMEM:   return QString("Error reading memory");
	case ERROR_WRITEMEM:  return QString("Error writing memory");
	case ERROR_COMM:      return QString("Communication error");
	case ERROR_MAX_RETRY: return QString("Too many failed attempts");
	case ERROR_TIMEOUT:   return QString("Timeout reached");
	case ERROR_MEMIDX:    return QString("Invalid memory index");
	}
	return QString("Unregistered error");
}

QString EEPROM::getCommandName(commands_e cmd) {
	switch(cmd)
	{
	case CMD_NONE:			return QString("None");
	case CMD_INIT:			return QString("Init");
	case CMD_PING:			return QString("Ping");
	case CMD_MEMID:			return QString("MemID");
	case CMD_IDLE:			return QString("Idle");
	case CMD_STARTXFER:		return QString("StartXfer");
	case CMD_ENDXFER:		return QString("EndXfer");
	case CMD_DISCONNECT:	return QString("Disconnect");
	case CMD_OK:			return QString("Ok");
	case CMD_TXRX_ACK:		return QString("XferAcknowledge");
	case CMD_TXRX_DONE:		return QString("XferDone");
	case CMD_ERR:			return QString("Error");
	case CMD_TXRX_ERR:		return QString("XferError");
	case CMD_READMEM:		return QString("ReadMemory");
	case CMD_READNEXT:		return QString("ReadNext");
	case CMD_MEMDATA:		return QString("MemoryData");
	case CMD_WRITEMEM:		return QString("WriteMemory");
	case CMD_DATA:			return QString("Data");
	case CMD_INFO:			return QString("Info");
	}
	return QString("Unregistered_command");
}

qint64 EEPROM::getMemSize(void) {
	return getMemSize(m_memtype);
}

qint64 EEPROM::getMemSize(memtype_e type) {
	switch(type) {
	case MEMTYPE_24LC16:  return 0x800;
	case MEMTYPE_24LC64:
	case MEMTYPE_X24645:  return 0x2000;
	case MEMTYPE_24LC256: return 0x8000;
	default: return -1;
	}
}

void EEPROM::setMem_X24645() {
	setTargetMem(MEMTYPE_X24645);
}
void EEPROM::setMem_24LC16() {
	setTargetMem(MEMTYPE_24LC16);
}
void EEPROM::setMem_24LC256() {
	setTargetMem(MEMTYPE_24LC256);
}
bool EEPROM::setTargetMem(memtype_e memtype) {
	if(memtype < MEMTYPE_24LC16 || memtype > MEMTYPE_mAX)
		return false;
	m_memtype = memtype;
	m_memsize = getMemSize(memtype);
	return true;
}
bool EEPROM::setTargetMem(QString memtype) {

	QString Memtype = memtype.toUpper();

	if(Memtype == "24LC16")
		m_memtype = MEMTYPE_24LC16;
	else if (Memtype == "X24645")
		m_memtype = MEMTYPE_X24645;
	else if (Memtype == "24LC64")
		m_memtype = MEMTYPE_24LC64;
	else if (Memtype == "24LC256")
		m_memtype = MEMTYPE_24LC256;
	else {
		return false;
	}

	m_memsize = getMemSize(m_memtype);
	return true;
}
