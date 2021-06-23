#include "serialportwriter.h"



SerialPortWriter::SerialPortWriter(QSerialPort *serialPort, QObject *parent) :
	QObject(parent),
	m_serialPort(serialPort),
	m_standardOutput(stdout)
{
	connect(m_serialPort, &QSerialPort::bytesWritten,
			this, &SerialPortWriter::handleBytesWritten);
	connect(m_serialPort, &QSerialPort::errorOccurred,
			this, &SerialPortWriter::handleError);
	connect(&m_timer, &QTimer::timeout,
			this, &SerialPortWriter::handleTimeout);

	m_timer.setSingleShot(true);
}

void SerialPortWriter::handleBytesWritten(qint64 bytes)
{
	m_bytesWritten += bytes;

	// TODO: this must be fucked up...
	if (m_bytesWritten == m_writeData.size()) {
//		m_standardOutput << QObject::tr("Data successfully sent to port %1:")
//							.arg(m_serialPort->portName()) << Qt::endl;
//		m_standardOutput << m_writeData.toHex() << Qt::endl;
		m_bytesWritten = 0;
		m_writeData.clear();
		m_timer.stop();
//		QCoreApplication::quit();
	}
}

void SerialPortWriter::handleTimeout()
{
	m_standardOutput << QObject::tr("Operation timed out for port %1, error: %2")
						.arg(m_serialPort->portName(), m_serialPort->errorString())
					 << Qt::endl;
	QCoreApplication::exit(1);
}

void SerialPortWriter::handleError(QSerialPort::SerialPortError serialPortError)
{
	if (serialPortError == QSerialPort::WriteError) {
		m_standardOutput << QObject::tr("An I/O error occurred while writing"
										" the data to port %1, error: %2")
							.arg(m_serialPort->portName(), m_serialPort->errorString())
						 << Qt::endl;
		QCoreApplication::exit(1);
	}
}

void SerialPortWriter::write(const char *writeData, qint64 len)
{
	m_writeData.append(writeData, len);

	const qint64 bytesWritten = m_serialPort->write(writeData, len);

	if (bytesWritten == -1) {
		m_standardOutput << QObject::tr("Failed to write the data to port %1, error: %2")
							.arg(m_serialPort->portName(), m_serialPort->errorString())
						 << Qt::endl;
		QCoreApplication::exit(1);
	} else if (bytesWritten != len) {
		m_standardOutput << QObject::tr("Failed to write all the data to port %1, error: %2")
							.arg(m_serialPort->portName(), m_serialPort->errorString())
						 << Qt::endl;
		QCoreApplication::exit(1);
	}
	enviados += bytesWritten;

	m_timer.start(2000+len); /* (?) */
}

void SerialPortWriter::write(const QByteArray &writeData)
{
	write(writeData.data(), writeData.length());
}

void SerialPortWriter::sendPackage(const char *cmd) {

	write((char *)cmd_startxfer, 3);
	write(cmd, 1);
	write((char *)cmd_endxfer, 3);
}

void SerialPortWriter::sendPackage(const char *cmd, const QByteArray &data) {

	write((char *)cmd_startxfer, 3);
	write(cmd, 1);
	write(data);
	write((char *)cmd_endxfer, 3);
}
