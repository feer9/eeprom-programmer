#include "serialportreader.h"


SerialPortReader::SerialPortReader(QSerialPort *serialPort,
								   FILE* outStream,
								   QObject *parent) :
	QObject(parent),
	m_serialPort(serialPort),
	m_standardOutput(outStream)
{
	connect(m_serialPort, &QSerialPort::readyRead,
			this, &SerialPortReader::handleReadyRead);
	connect(m_serialPort, &QSerialPort::errorOccurred,
			this, &SerialPortReader::handleError);
	connect(&m_timer, &QTimer::timeout,
			this, &SerialPortReader::handleTimeout);

	m_readData.reserve(10000);
	m_timer.setSingleShot(true);
//	m_timer.start(5000);
}

void SerialPortReader::clearBuffer() {
	m_readData.clear();
	m_state = 0;
}

void SerialPortReader::startRxTimeout(int time_ms) {
	m_timer.start(time_ms);
}
void SerialPortReader::stopRxTimeout() {
	m_timer.stop();
}

void SerialPortReader::handleTimeout() {
//	clearBuffer();
//	emit timeout();
}

void SerialPortReader::handleError(QSerialPort::SerialPortError serialPortError)
{
	if (serialPortError == QSerialPort::ReadError) {
		m_standardOutput << QObject::tr("An I/O error occurred while reading "
										"the data from port %1, error: %2")
							.arg(m_serialPort->portName(), m_serialPort->errorString())
						 << Qt::endl;
		QCoreApplication::exit(1);
	}
}

void SerialPortReader::handleReadyRead()
{
	qDebug() << QObject::tr("Received %1 bytes of data")
						.arg(m_serialPort->bytesAvailable());

	QByteArray recv = m_serialPort->readAll();
	m_received += recv.length();
	m_readData.append(recv);
	m_available = m_readData.length();

	processRx();

	// restart timeout each time we get something
	m_timer.start();
}

void SerialPortReader::processRx(void)
{
	bool pkg_ready = false;

	while(!m_readData.isEmpty())
	{
		uint8_t byte;
		switch(m_state)
		{
		case 0: /* <STX> */
			byte = m_readData[0];
			m_readData.remove(0,1);
			if(byte == CMD_STARTXFER)
				++m_state;
			break;

		case 1: /* <COMMAND> */
			byte = m_readData[0];
			m_pkg.cmd = static_cast<commands_e>(char(byte));
			m_readData.remove(0,1);
			m_pkgData.clear();
			m_pkg.datalen = qMin(EEPROM::cmdHasData(m_pkg.cmd), PKG_DATA_MAX);
			if(m_pkg.datalen != 0)
				++m_state;
			else
				m_state = 3;
			break;

		case 2: /* <DATA> */
			qint64 bytesLeft;
			bytesLeft = m_pkg.datalen - m_pkgData.size();
			m_pkgData.append(m_readData.left(bytesLeft));
			m_readData.remove(0, bytesLeft);
			if(m_pkgData.size() == m_pkg.datalen) {
				m_pkg.data = (uint8_t *)(m_pkgData.data());
				++m_state;
			}
			break;

		case 3: /* <CHECKSUM> */
			if(m_readData.length() < 2) {
				return;
			}
			else {
				QByteArray tmp = m_readData.left(2);
				m_readData.remove(0,2);
				m_pkg.crc = CRC16::arrayToWord(tmp);
				++m_state;
			}
			break;

		case 4: /* <ETX> */
			byte = m_readData[0];
			m_readData.remove(0,1);
			if(byte == CMD_ENDXFER)
				pkg_ready = true;

			[[fallthrough]];
		default:
			m_state = 0;
			break;
		}

		if(pkg_ready)
		{
			if(m_readData.isEmpty())
				m_timer.stop();
			emit packageReady(&m_pkg);
		}
	}
}

