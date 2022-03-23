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

#include "memorycomm.h"


MemoryComm::MemoryComm(int &argc, char **argv, FILE* outStream)
	: QCoreApplication(argc, argv)
	, m_buffer()
	, m_pkg(pkgdata_t({CMD_NONE,m_buffer}))
	, m_standardOutput(outStream)
	, m_serialPortWriter(&m_serialPort, outStream)
	, m_serialPortReader(&m_serialPort, outStream)
{
//	m_pkg.cmd = CMD_NONE;
//	m_pkg.data = &m_buffer;

	setSignals();
}

void MemoryComm::setSignals()
{
	connect(&m_serialPortReader, &SerialPortReader::packageReady,
						   this, &MemoryComm::handlePackageReceived);

	connect(&m_serialPortReader, &SerialPortReader::timeout,
						   this, &MemoryComm::handleRxTimedOut);

	connect(&m_serialPortWriter, &SerialPortWriter::packageSent,
						   this, &MemoryComm::handlePackageSent);
}


void MemoryComm::setSerialPortOptions(SerialPortOptions& op)
{
	m_serialPort.setPortName(op.name);
	m_serialPort.setBaudRate(op.baudrate);
	m_serialPort.setDataBits(op.databits);
	m_serialPort.setParity(op.parity);
	m_serialPort.setStopBits(op.stopbits);
	m_serialPort.setFlowControl(op.flowcontrol);
}

MemoryComm::~MemoryComm()
{
	if(m_serialPort.isOpen()) {
		qDebug() <<  "SerialPort connected. Sending CMD_DISCONNECT...";
		sendCommand(CMD_DISCONNECT);
		m_serialPort.waitForBytesWritten(500);
		m_serialPort.close();
	}
	else {
		qDebug() << "SerialPort not connected.";
	}
}

void MemoryComm::clearBuffers() {
	m_serialPortReader.clearBuffer();
	m_buffer.clear();
}


bool MemoryComm::sendCommand(commands_e cmd, uint8_t data) {
	return sendCommand(cmd, QByteArray(1, char(data)));
}

bool MemoryComm::sendCommand(commands_e cmd) {
	return sendCommand(cmd, QByteArray());
}

// return true on success, false otherwise
bool MemoryComm::sendCommand(commands_e cmd, const QByteArray& data) {

	qDebug() << Qt::endl << "about to send command"
			 << EEPROM::getCommandName(cmd) << "with"
			 << data.size() << "bytes of data.";

	m_lastTxCmd = cmd;

	qint64 ret = m_serialPortWriter.send(cmd, data);

	bool success = ret != -1;
	if(success) {
		setRxTimeout(cmd);
	}
	else {
		qDebug() << "Error sending command" << EEPROM::getCommandName(cmd);
	}

	return success;
}

bool MemoryComm::readMem() {

	m_buffer.clear();

	m_operation = OP_RX;
	m_commState = COMM_READMEM_WAIT_OK;

	return sendCommand(CMD_READMEM, m_memtype);
}

bool MemoryComm::writeMem(const QByteArray& memBuffer) {

	m_operation = OP_TX;
	m_commState = COMM_WRITEMEM_WAIT_OK;
	m_memindex = 0;
	m_memBuffer = memBuffer; // CHECK HOW THIS WORKS

	m_serialPortReader.clearBuffer();
	m_serialPort.clear(QSerialPort::Input);

	return sendCommand(CMD_WRITEMEM, m_memtype);
}

bool MemoryComm::sendMemoryBlock() {
	int memidx = m_memindex;
	m_memindex += PKG_DATA_MAX;

	return sendCommand(CMD_MEMDATA, m_memBuffer.mid(memidx, PKG_DATA_MAX));
}

bool MemoryComm::sendCommand_ping(void) {
	return sendCommand(CMD_PING);
}

void MemoryComm::handleRxCrcError()
{
	// see m_lastRxCmd and m_lastTxCmd
}

void MemoryComm::reconnect(void) {
/*	m_serialPort.close();
	if (!m_serialPort.open(QIODevice::ReadWrite)) {
		m_standardOutput << "Unable to open serial port. Aborting." << Qt::endl;
		QCoreApplication::exit(1);
	}*/
}

void MemoryComm::handleRxTimedOut() {

	// chequear si no estaba esperando nada no le des bola a este timeout

	if(m_operation == OP_RX) {
		m_standardOutput << "Reading from memory timed out." << Qt::endl;
		reconnect();
	}
	else if(m_operation == OP_TX) {
		m_standardOutput << "Writing to memory timed out." << Qt::endl;
		reconnect();
	}
	else {
		m_standardOutput << "RX timed out." << Qt::endl;
	}
	m_operation = OP_NONE;
}

void MemoryComm::errorReceived(package_t *pkg)
{
	if(pkg->cmd == CMD_ERR) {
		m_standardOutput << "ReadMem: uC error: "
						 << EEPROM::getErrorMsg(errorcode_e(pkg->data[0]))
						 << Qt::endl;
	}
	m_commState = COMM_IDLE;
	m_operation = OP_NONE;
	// forward error to application
	packageReady(pkg);
}

void MemoryComm::handlePackageReceived(package_t *pkg)
{
	qDebug() << "Received command" << EEPROM::getCommandName(pkg->cmd);

	if(!CRC16::check(pkg))
	{
		handleRxCrcError();
	}
	else
	{
		switch(m_commState)
		{
		case COMM_IDLE: // not in transfer
			// just forward package to application
			packageReady(pkg);
			break;

		case COMM_READMEM_WAIT_OK: // readmem sent, waiting confirmation
			if(pkg->cmd == CMD_OK) {
				sendCommand(CMD_READNEXT);
				m_commState = COMM_READMEM_WAIT_DATA;
			}
			else {
				errorReceived(pkg);
			}
			break;

		case COMM_READMEM_WAIT_DATA:
			if(pkg->cmd == CMD_MEMDATA)
			{
				m_buffer.append((char*)(pkg->data), pkg->datalen);
				qDebug("Received %d bytes out of %d", m_buffer.size(), m_memsize);
				if(m_buffer.size() < m_memsize) {
					sendCommand(CMD_TXRX_ACK);
					m_pending.append(CMD_READNEXT);
				}
				else {
					m_commState = COMM_IDLE;
					m_operation = OP_NONE;
					sendCommand(CMD_TXRX_DONE);
					pkg->data = (uint8_t*)(m_buffer.data());
					pkg->datalen = m_buffer.size();
					packageReady(pkg);
				}
			}
			else {
				errorReceived(pkg);
			}
			break;

		case COMM_WRITEMEM_WAIT_OK:
			if(pkg->cmd == CMD_OK) {
				sendMemoryBlock();
				m_commState = COMM_WRITEMEM_WAIT_ACK;
			}
			else {
				errorReceived(pkg);
			}
			break;
		case COMM_WRITEMEM_WAIT_ACK:
			qDebug("Sent %d bytes out of %d", m_memindex, m_memsize);
			if(pkg->cmd == CMD_TXRX_ACK) {
				if(m_memindex >= m_memsize) {
					// uC is doing some unintelligent thing
					setPackageError(pkg, ERROR_MEMIDX);
					errorReceived(pkg);
				}
				else {
					if(!sendMemoryBlock()) {
						setPackageError(pkg, ERROR_COMM);
						errorReceived(pkg);
					}
				}
			}
			else if(pkg->cmd == CMD_TXRX_DONE && m_memindex == m_memsize) {

				m_commState = COMM_IDLE;
				m_operation = OP_NONE;
				packageReady(pkg);
			}
			else {
				errorReceived(pkg);
			}
			break;
		}
	}

	m_lastRxCmd = pkg->cmd;
}
// TODO: split in methods.

void MemoryComm::setPackageError(package_t *pkg, errorcode_e err)
{
	pkg->cmd = CMD_ERR;
	pkg->datalen = 1;
	m_buffer[0] = err;
	pkg->data = (uint8_t*)(m_buffer.data());
}

void MemoryComm::packageReady(package_t *pkg)
{
	// hasta acá el paquete llega bien.
	m_pkg.cmd = pkg->cmd;
	QByteArray data((char*)(pkg->data), pkg->datalen);
	m_pkg.data = data;

	handleXfer(&m_pkg);
}

void MemoryComm::handlePackageSent(commands_e cmd)
{
	qDebug() << "Finished sending command: " << EEPROM::getCommandName(cmd);

	if(!m_pending.isEmpty()) {
		uint8_t command = m_pending[0];
		m_pending.remove(0,1);
		sendCommand(commands_e(command));
		// TODO: this doesn't account for command data
	}
}

// when we send <cmd>, expect an answer in X time
void MemoryComm::setRxTimeout(commands_e cmd)
{
	switch(cmd)
	{
	case CMD_NONE:
	case CMD_STARTXFER:
	case CMD_ENDXFER:
	case CMD_IDLE:
		break;

	case CMD_INIT:
		m_serialPortReader.startRxTimeout(2000);
		break;
	case CMD_PING:
	case CMD_MEMID:
	case CMD_DATA:
		m_serialPortReader.startRxTimeout(200);
		break;

	case CMD_DISCONNECT:
	case CMD_OK:
	case CMD_ERR:
	case CMD_TXRX_DONE:
		m_serialPortReader.stopRxTimeout();
		break;

	case CMD_TXRX_ACK:
	case CMD_TXRX_ERR:
	case CMD_READNEXT:
	case CMD_MEMDATA:
	case CMD_INFO:
		m_serialPortReader.startRxTimeout(1500);
		break;

	case CMD_READMEM:
	case CMD_WRITEMEM:
		m_serialPortReader.startRxTimeout(7000);
		break;
	// TODO: check all this timing thing
	}
}
