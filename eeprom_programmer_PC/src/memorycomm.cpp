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

//	connect(&m_serialPortWriter, &SerialPortWriter::txXferComplete,
//						   this, &MemoryComm::handleTxXferComplete);

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

	qDebug() << "about to send command: " << EEPROM::getCommandName(cmd);
	m_lastTxCmd = cmd;

	qint64 ret = m_serialPortWriter.send(cmd, data);

	bool success = ret != -1;
	if(success) {
		setRxTimeout(cmd);
	}
	else {
		qDebug() << "Error sending command " << EEPROM::getCommandName(cmd);
	}

	return success;
}

bool MemoryComm::readMem() {

	m_buffer.clear();
//	m_buffer.resize(m_memsize);

	m_xferMode = MODE_RX;
	m_xferState = CMD_READMEM;

	return sendCommand(CMD_READMEM, m_memtype);

//	if(!sendCommand(CMD_READNEXT))
//		return false;
}

bool MemoryComm::writeMem(const QByteArray& memBuffer) {

	m_xferMode = MODE_TX;
	m_xferState = CMD_WRITEMEM;

	m_serialPortReader.clearBuffer();
	m_serialPort.clear(QSerialPort::Input);

	if(!sendCommand(CMD_WRITEMEM, m_memtype))
		return false;
	if(!sendCommand(CMD_MEMDATA, memBuffer))
		return false;
	// todo: can't send two commands together

	return true;
}


bool MemoryComm::sendCommand_ping(void) {
	return sendCommand(CMD_PING);
}

void MemoryComm::handleRxCrcError()
{
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

	if(m_xferMode == MODE_RX) {
		m_standardOutput << "Reading from memory timed out." << Qt::endl;
		reconnect();
	}
	else if(m_xferMode == MODE_TX) {
		m_standardOutput << "Writing to memory timed out." << Qt::endl;
		reconnect();
	}
	else {
		m_standardOutput << "RX timed out." << Qt::endl;
	}
	m_xferMode = MODE_NONE;
}

void MemoryComm::errorReceived(package_t *pkg)
{
	if(pkg->cmd == CMD_ERR) {
		m_standardOutput << "ReadMem: uC error: "
						 << EEPROM::getErrorMsg(errorcode_e(pkg->data[0]))
						 << Qt::endl;
	}
	m_xferState = 0;
	m_xferMode = MODE_NONE;
	// forward error to application
	packageReady(pkg);
}

void MemoryComm::handlePackageReceived(package_t *pkg) {

	qDebug() << "Received command: " << EEPROM::getCommandName(pkg->cmd);

	if(!CRC16::check(pkg))
	{
		handleRxCrcError();
	}
	else
	{
		switch(m_xferState)
		{
		case 0: // not in transfer
			// just forward package to application
			packageReady(pkg);
			break;

		case CMD_READMEM: // readmem sent, waiting confirmation
			if(pkg->cmd == CMD_OK) {
				commands_e cmd = CMD_READNEXT;
				sendCommand(cmd);
				m_xferState = cmd;
			}
			else {
				errorReceived(pkg);
			}

			break;

		case CMD_READNEXT:
			if(pkg->cmd == CMD_MEMDATA)
			{
				m_buffer.append((char*)(pkg->data), pkg->datalen);
				qDebug("Received %d bytes out of %d", m_buffer.size(), m_memsize);
				if(m_buffer.size() < m_memsize) {
					sendCommand(CMD_TXRX_ACK);
					m_pending.append(CMD_READNEXT);
				}
				else {
					m_xferState = 0;
					m_xferMode = MODE_NONE;
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
		}
	}

	m_lastRxCmd = pkg->cmd;
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
//		m_pending.left(1);
		uint8_t command = m_pending[0];
		m_pending.remove(0,1);
		sendCommand(commands_e(command));
	}
}

/*void MemoryComm::handleTxXferComplete(int status) {
	// we won't receive anything else
	m_serialPortReader.stopRxTimeout();

	if(status != CMD_TXRX_ACK) {
//		m_xferState = 0;
	}

	handleXfer(nullptr);
}*/

void MemoryComm::setRxTimeout(commands_e cmd) {

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
		m_serialPortReader.startRxTimeout(1500);
		break;

	case CMD_READMEM:
	case CMD_WRITEMEM:
		m_serialPortReader.startRxTimeout(7000);
		break;
	// TODO: CHECK DIS SHIT
	}
}
