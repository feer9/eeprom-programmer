/*
 *  EEPROM-Programmer - Read and write EEPROM memories.
 *  Copyright (C) 2022  Fernando Coda <fcoda@pm.me>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "app.h"


void App::reconnect()
{
	m_xferState = ST_DISCONNECTED;
	m_connected = false;
	handleXfer(nullptr);
}

void App::handleTimeout(void) {
	m_standardOutput << "uC connection timed out." << Qt::endl;
	reconnect();
}

void App::pingTimerLoop(void) {
/*	if(m_xferState == ST_IDLE && m_currentOperation == OP_NONE)
	{
		sendCommand_ping();
		m_xferState = ST_PING;
	}*/
	setNextOperation(OP_PING);
	handleXfer(nullptr);
}

void App::printError(pkgdata_t *pkg)
{
	if(!pkg || pkg->cmd != CMD_ERR)
		m_standardOutput << "Unknown error from uC" << Qt::endl;
	else {
		char c_err = pkg->data[0];
		errorcode_e err = errorcode_e(c_err);
		m_standardOutput << "uC ERROR " << EEPROM::getErrorMsg(err) << Qt::endl;
	}
}

bool App::doSomething()
{
	// We only process the new request
	// if there's nothing being done.
	if(m_currentOperation != OP_NONE)
		return false;

	m_xferState = ST_IDLE;

	switch(m_nextOperation)
	{
	case OP_RX:
		m_currentOperation = m_nextOperation;
		m_xferState = ST_WAIT_READMEM;
		readMem();
		break;

	case OP_TX:
		m_currentOperation = m_nextOperation;
		m_xferState = ST_WAIT_WRITEMEM;
		if(!App::writeMem()) {
			m_currentOperation = OP_NONE;
			m_xferState = ST_IDLE;
		}
		break;

	case OP_PING:
		m_xferState = ST_PING;
		sendCommand_ping();
		break;

	case OP_NONE:
		break;

	default:
		m_standardOutput << "Invalid operation." << Qt::endl;
		break;
	}

	// clean "Next" flag
	m_nextOperation = OP_NONE;


	// return true if we're gonna do something
	return m_currentOperation != OP_NONE;
}

void App::handleXfer(pkgdata_t *pkg) {
	// TODO: handle error packages

	switch(m_xferState)
	{
	case ST_DISCONNECTED: // trying to connect
		m_connected = false;
		sendCommand(CMD_INIT);
		m_xferState = ST_INIT;
		break;

	case ST_INIT: // Waiting for INIT response

		if(!pkg)
			break;

		if(pkg->cmd == CMD_INIT) {
			m_standardOutput << "Connected to uC." << Qt::endl;
			m_connected = true;
			m_xferState = ST_IDLE;
			doSomething();
		}
		else {
			m_standardOutput << "Could not connect to uC" << Qt::endl;
			clearBuffers();
			m_xferState = ST_DISCONNECTED;
		}
		break;

	case ST_IDLE:
		doSomething();
		break;

	case ST_PING: // waiting for ping answer

		if(!pkg)
			break;

		if(pkg->cmd == CMD_TXRX_ACK) {
			m_standardOutput << "Ping." << Qt::endl;
			m_xferState = ST_IDLE;
		}
		else {
			m_standardOutput << "Connection error" << Qt::endl;
			m_connected = false;
			m_xferState = ST_DISCONNECTED;
		}
		break;

	case ST_WAIT_READMEM: // requested read eeprom - waiting memory data

		if(!pkg)
			break;

		if(pkg->cmd == CMD_MEMDATA) {
			// finished receiving memory data
			m_memBuffer = pkg->data;
			printData();
			saveData();
			m_currentOperation = OP_NONE;
		}
		else {
			printError(pkg);
		}
		clearBuffers();
		m_xferState = ST_IDLE;
QTimer::singleShot(50, qApp, SLOT(quit()));
		break;

	case ST_WAIT_WRITEMEM: // requested eeprom full write - waiting confirmation

		if(!pkg)
			break;

		if(pkg->cmd == CMD_TXRX_DONE) {
			m_standardOutput << "Memory write SUCCESSFULLY" << Qt::endl;
			m_currentOperation = OP_NONE;
		}
		else {
			printError(pkg);
		}
		clearBuffers();
		m_xferState = ST_IDLE;
QTimer::singleShot(50, qApp, SLOT(quit()));
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

