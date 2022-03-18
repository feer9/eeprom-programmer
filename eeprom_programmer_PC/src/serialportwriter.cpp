#include "serialportwriter.h"



SerialPortWriter::SerialPortWriter(QSerialPort *serialPort,
								   FILE* outStream,
								   QObject *parent)
	: QObject(parent)
	, m_standardOutput(outStream)
	, m_serialPort(serialPort)
{
	setSignals();

	m_timer.setSingleShot(true);
}

void SerialPortWriter::setSignals()
{
	connect(m_serialPort, &QSerialPort::bytesWritten,
			this, &SerialPortWriter::handleBytesWritten);
	connect(m_serialPort, &QSerialPort::errorOccurred,
			this, &SerialPortWriter::handleError);
	connect(&m_timer, &QTimer::timeout,
			this, &SerialPortWriter::handleTimeout); // is this being used?
}

void SerialPortWriter::handleTimeout()
{
	m_standardOutput << QObject::tr("Operation timed out for port %1: %2")
						.arg(m_serialPort->portName(), m_serialPort->errorString())
					 << Qt::endl;
	QCoreApplication::exit(1);
}

void SerialPortWriter::handleError(QSerialPort::SerialPortError serialPortError)
{
	if (serialPortError == QSerialPort::WriteError) {
		m_standardOutput << QObject::tr("An I/O error occurred while writing"
										" the data to port %1: %2")
							.arg(m_serialPort->portName(), m_serialPort->errorString())
						 << Qt::endl;
		QCoreApplication::exit(1);
	}
}

void SerialPortWriter::finishTxXfer(int status)
{
	m_busy = false;
	m_tries = 0;
	emit txXferComplete(status);
}

/*void SerialPortWriter::transmitNextPackage()
{
	m_tries = 0;
	m_bytesRemaining -= PKG_DATA_MAX;

	if (m_bytesRemaining == 0)
	{
		finishTxXfer(CMD_TXRX_ACK);
	}
	else
	{
		int pos = m_data.size() - m_bytesRemaining;
		m_packageData = m_data.mid(pos, PKG_DATA_MAX);
		sendPackage();
	}
}

void SerialPortWriter::targetRxError()
{
	if(++m_tries < 3)
		sendPackage();
//		send(m_cmd, m_data);

	else
		finishTxXfer(CMD_TXRX_ERR);
}

void SerialPortWriter::handleTargetReportRxStatus(int status)
{
	if(status == CMD_TXRX_ACK)
		transmitNextPackage();

	else
		targetRxError();
}*/

// QSerialPort callback from signal bytesWritten
void SerialPortWriter::handleBytesWritten(qint64 bytes)
{
	m_totalBytesSent += bytes;
	m_packageBytesWritten += bytes;

	if (m_packageBytesWritten == m_package.size())
	{
		qDebug() << "Finished sending data.";
//		qDebug() << m_writeData.toHex();

		m_busy = false;
		m_packageBytesWritten = 0;
		m_package.clear();
		m_timer.stop();

		emit packageSent(m_cmd);
	}
}

qint64 SerialPortWriter::write()
{
	const qint64 bytesWritten = m_serialPort->write(m_package);

	if (bytesWritten == -1) {
		m_standardOutput << QObject::tr("Failed to write the data to port %1: %2")
							.arg(m_serialPort->portName(), m_serialPort->errorString())
						 << Qt::endl;
	}
	else {
		m_timer.start(2000+m_package.size()); /* internal timer for LOCAL timeout */
	}
	return bytesWritten;
}

qint64 SerialPortWriter::sendPackage(void)
{
	m_busy = true;
	m_package.clear();
	m_package.append(char(CMD_STARTXFER));
	m_package.append(m_cmd);
	if(!m_packageData.isEmpty()) {
		m_package.append(m_packageData);
		m_package.append(CRC16::genByteArray(m_cmd, m_packageData));
	}
	else {
		m_package.append(CRC16::genByteArray(m_cmd));
	}
	m_package.append(char(CMD_ENDXFER));

	return write();
}

/* ---------------- Public Methods ------------------ */

qint64 SerialPortWriter::send(commands_e cmd, const QByteArray &data) {

	if(m_busy)
		return -1;
	m_busy = true;

	m_cmd = cmd;
	m_data = data;
	m_packageData = data.left(PKG_DATA_MAX);
	m_bytesRemaining = data.size();

	return sendPackage();
}

qint64 SerialPortWriter::send(commands_e cmd) {

	if(m_busy)
		return -1;
	m_busy = true;

	m_cmd = cmd;
	m_data.clear();
	m_packageData.clear();
	m_bytesRemaining = 0;

	return sendPackage();
}

bool SerialPortWriter::busy() const
{
	return m_busy;
}
