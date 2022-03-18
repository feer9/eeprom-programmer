#include "memorycomm.h"


MemoryComm::MemoryComm(int &argc, char **argv, FILE* outStream)
	: QCoreApplication(argc, argv)
	, m_pkg(pkgdata_t({CMD_NONE,&m_buffer}))
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

//	connect(&m_serialPortReader, &SerialPortReader::crcError,
//						   this, &MemoryComm::handleRxCrcError);

//	connect(&m_serialPortReader, &SerialPortReader::targetReportsRxStatus,
//			&m_serialPortWriter, &SerialPortWriter::handleTargetReportRxStatus);

//	connect(&m_serialPortReader, &SerialPortReader::rxInProgress,
//			&m_serialPortWriter, &SerialPortWriter::sendRxAck);

	connect(&m_serialPortWriter, &SerialPortWriter::txXferComplete,
						   this, &MemoryComm::handleTxXferComplete);

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
		sendCommand(CMD_DISCONNECT);
		m_serialPort.waitForBytesWritten(500);
		m_serialPort.close();
	}
}

void MemoryComm::clearBuffers() {
	m_serialPortReader.clearBuffer();
	m_buffer.clear();
}


void MemoryComm::sendCommand(commands_e cmd, uint8_t data) {
	sendCommand(cmd, QByteArray(1, char(data)));
}

void MemoryComm::sendCommand(commands_e cmd) {
	sendCommand(cmd, QByteArray());
}

void MemoryComm::sendCommand(commands_e cmd, const QByteArray& data) {

	m_lastTxCmd = cmd;

	if(data.isNull()) {
		m_serialPortWriter.send(cmd);
	}
	else {
		m_serialPortWriter.send(cmd, data);
	}
	// todo: check return status of send; return bool status
}

void MemoryComm::readMem() {

	m_buffer.clear();
	m_buffer.resize(m_memsize);

	m_xferMode = MODE_RX;
	m_xferState = CMD_READNEXT;

	sendCommand(CMD_READMEM, m_memtype);
	sendCommand(CMD_READNEXT);
}

bool MemoryComm::writeMem(const QByteArray& memBuffer) {

	m_xferMode = MODE_TX;
	m_xferState = CMD_WRITEMEM;

	m_serialPortReader.clearBuffer();
	m_serialPort.clear(QSerialPort::Input);

	sendCommand(CMD_WRITEMEM, m_memtype);
	sendCommand(CMD_MEMDATA, memBuffer);

	return true;
}


void MemoryComm::sendCommand_ping(void) {
	sendCommand(CMD_PING);
}

void MemoryComm::handleRxCrcError()
{
}

void MemoryComm::reconnect(void) {
	m_serialPort.close();
	if (!m_serialPort.open(QIODevice::ReadWrite)) {
		m_standardOutput << "Unable to open serial port. Aborting." << Qt::endl;
		QCoreApplication::exit(1);
	}
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
}

void MemoryComm::handlePackageReceived(package_t *pkg) {

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

		case CMD_READMEM:
		case CMD_READNEXT:
			if(pkg->cmd == CMD_MEMDATA)
			{
				m_buffer.append((char*)(pkg->data), pkg->datalen);
				if(m_buffer.size() < m_memsize) {
					sendCommand(CMD_TXRX_ACK);
				}
				else {
					m_xferState = 0;
					sendCommand(CMD_TXRX_DONE);
					pkg->data = (uint8_t*)(m_buffer.data());
					packageReady(pkg);
				}
			}

			break;
		}
	}

	m_lastRxCmd = pkg->cmd;
}

void MemoryComm::packageReady(package_t *pkg)
{
	m_pkg.cmd = pkg->cmd;
	m_pkg.data->setRawData((char*)(pkg->data), pkg->datalen);

	handleXfer(&m_pkg);
}

void MemoryComm::handleTxXferComplete(int status) {
	// we won't receive anything else
	m_serialPortReader.stopRxTimeout();

	if(status != CMD_TXRX_ACK)
//		m_xferState = 0;


	handleXfer(nullptr);
}

void MemoryComm::handlePackageSent(commands_e cmd) {

	switch(cmd)
	{
	case CMD_NONE:
	case CMD_STARTXFER:
	case CMD_ENDXFER:
		break;

	case CMD_INIT:
	case CMD_PING:
	case CMD_MEMID:
		m_serialPortReader.startRxTimeout(1500);
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
	}
}
