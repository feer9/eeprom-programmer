#include "app.h"


void App::reconnect()
{
	m_xferState = 0;
	m_connected = false;
	handleXfer(nullptr);
}

void App::handleTimeout(void) {
	m_standardOutput << "uC connection timed out." << Qt::endl;
	reconnect();
}

void App::pingTimerLoop(void) {
	if(m_xferState == CMD_IDLE && m_requestedOperation == CMD_NONE)
	{
		sendCommand_ping();
		m_xferState = CMD_PING;
	}
}

void App::printError(pkgdata_t *pkg)
{
	if(!pkg || pkg->cmd != CMD_ERR)
		m_standardOutput << "Unknown error from uC" << Qt::endl;
	else
		m_standardOutput << "uC ERROR " << pkg->data[0] << " reading data..." << Qt::endl;
}

void App::doStuff()
{
	if(m_requestedOperation == CMD_NONE) {
		m_xferState = CMD_IDLE;
	}
	else {
		// Send read or write memory command
		if(m_requestedOperation == CMD_READMEM) {
			readMem();
			m_xferState = CMD_READMEM;
		}
		else if(m_requestedOperation == CMD_WRITEMEM) {
			if(writeMem()) {
				m_xferState = CMD_WRITEMEM;
			}
			else {
				m_requestedOperation = CMD_NONE;
				m_xferState = CMD_IDLE;
			}
		}
		else {
			m_standardOutput << "Invalid operation." << Qt::endl;
			m_requestedOperation = CMD_NONE;
			m_xferState = CMD_IDLE;
		}
	}
}

void App::handleXfer(pkgdata_t *pkg) {

	switch(m_xferState)
	{
	case 0: // trying to connect
		m_connected = false;
		sendCommand(CMD_INIT);
		m_xferState = 1;
		break;

	case 1: // Waiting for INIT response

		if(!pkg)
			break;

		if(pkg->cmd == CMD_INIT) {
			m_standardOutput << "Connected to uC." << Qt::endl;
			m_connected = true;
			// Make a call to the uC
			doStuff();
		}
		else {
			m_standardOutput << "Could not connect to uC" << Qt::endl;
			clearBuffers();
			m_xferState = 0;
		}
		break;
//		[[fallthrough]];

	case CMD_IDLE: // not doing much :-)
		break;

	case CMD_PING: // waiting for ping answer

		if(!pkg)
			break;

		if(pkg->cmd == CMD_TXRX_ACK) {
			m_standardOutput << "Ping." << Qt::endl;
			m_xferState = CMD_IDLE;
		}
		else {
			m_standardOutput << "Connection error" << Qt::endl;
			m_connected = false;
			m_xferState = 0;
		}
		break;

	case CMD_READMEM: // requested read eeprom - waiting memory data

		if(!pkg)
			break;

		if(pkg->cmd == CMD_MEMDATA) {
			// finished receiving memory data

			m_memBuffer = pkg->data;
			printData();
			saveData();
			m_requestedOperation = CMD_NONE;
		}
		else {
			printError(pkg);
		}
		clearBuffers();
		m_xferState = CMD_IDLE;
		break;

	case CMD_WRITEMEM: // requested eeprom full write - waiting confirmation

		if(!pkg)
			break;

		if(pkg->cmd == CMD_OK) { // TODO: check what cmd should be
			m_standardOutput << "Memory write SUCCESSFULLY" << Qt::endl;
			m_requestedOperation = CMD_NONE;
		}
		else {
			printError(pkg);
		}
		clearBuffers();
		m_xferState = CMD_IDLE;
		break;

	default:
		while(1); // catch the bug :-)
	}
}
// TODO: split into simple methods




void App::printData() {

//	m_standardOutput << m_readData.toHex() << Qt::endl;
	int i;
	char buf[128];
//	QByteArray buff[128];
//	QByteArray::Iterator it = buff->begin();
	uint8_t *page = (uint8_t*)(m_memBuffer.data());

	for(int reg=0; reg < m_memsize; reg += 32)
	{
		char *p = buf;
		sprintf(p, "%04X: ", reg );
		p+=6;
		for(i=0; i<16; ++i) {
			sprintf(p+3*i, "%02X ", int(page[reg+i]) );
		}
		m_standardOutput << buf << Qt::endl;

		p=buf;
		sprintf(p, "%04X: ", reg+16 );
		p+=6;
		for(i=0; i<16; ++i) {
			sprintf(p+3*i, "%02X ", int(page[16+reg+i]) );
		}
		m_standardOutput << buf << Qt::endl;
	}
	// No time for doing better
}


bool App::saveData() {

	QFile file(m_filename_out);

	if (!file.open(QIODevice::WriteOnly)) {
		m_standardOutput << "Could not open file \"" << m_filename_out
						 << "\" for writing." << Qt::endl;
		return false;
	}

	file.write(m_memBuffer);
	file.close();
	m_standardOutput << "Saved data to file \"" << m_filename_out
					 << "\"." << Qt::endl;
	return true;
}


bool App::writeMem() {

	QFile file((m_filename_in));

	if (!file.open(QIODevice::ReadOnly)) {
		m_standardOutput << "Could not open file \"" << m_filename_in
						 << "\" for reading." << Qt::endl;
		return false;
	}

	if(file.size() != m_memsize) {
		m_standardOutput << "File size don't match." << Qt::endl;
		return false;
	}

	m_memBuffer.clear();
	m_memBuffer.resize(m_memsize);
	m_memBuffer = file.readAll();
	file.close();

	return MemoryComm::writeMem(m_memBuffer);
}

