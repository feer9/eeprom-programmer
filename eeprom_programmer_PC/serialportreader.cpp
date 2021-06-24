#include "serialportreader.h"


SerialPortReader::SerialPortReader(QSerialPort *serialPort,
								   QObject *parent) :
	QObject(parent),
	m_serialPort(serialPort),
	m_standardOutput(stdout)
{
	connect(m_serialPort, &QSerialPort::readyRead,
			this, &SerialPortReader::handleReadyRead);
	connect(m_serialPort, &QSerialPort::errorOccurred,
			this, &SerialPortReader::handleError);
	connect(&m_timer, &QTimer::timeout,
			this, &SerialPortReader::handleTimeout);

	m_readData.reserve(10000);
	m_timer.setSingleShot(true);
	m_timer.start(5000);
}

void SerialPortReader::clearBuffer() {
	m_readData.clear();
}

// attempt to remove wrong data into the RX buffer
void SerialPortReader::cleanBuffer() {

	m_readData.clear();
//	int len = m_readData.length();
//	int last = len - 1;

//	if(len == 0)
//		return;


/*	if(uint8_t(m_readData[last]) == cmd_init[3] ||
	   uint8_t(m_readData[last]) == cmd_startxfer[3]) {
		m_readData.clear();
	}
	else */
//	if(len > 4) {
//		m_readData.remove(0, len-4);
//		last = 3;
//	}
	// wtf is this code
}




void SerialPortReader::handleReadyRead()
{
//	m_standardOutput << QObject::tr("Received %1 bytes of data")
//						.arg(m_serialPort->bytesAvailable()) << Qt::endl;

	QByteArray recv = m_serialPort->readAll();
	m_recibidos += recv.length();
	m_readData.append(recv);
	m_available = m_readData.length();

	emit CallXferHandler(m_readData);
	m_timer.start(); // restart timeout each time we get something
}



void SerialPortReader::handleTimeout()
{
	clearBuffer();

	emit CallXferTimedOut();
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
