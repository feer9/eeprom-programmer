#ifndef SERIALPORTREADER_H
#define SERIALPORTREADER_H

#include <QCoreApplication>
#include <QByteArray>
#include <QSerialPort>
#include <QStringList>
#include <QTimer>
#include <QDebug>

#include "eeprom.h"
#include "crc16.h"

class SerialPortWriter;

class SerialPortReader : public QObject
{
	Q_OBJECT

protected:

public:
	SerialPortReader(QSerialPort *serialPort,
					 FILE* outStream,
					 QObject *parent = nullptr);

	virtual ~SerialPortReader() {qDebug() << "Data received: " << m_received; };

	void clearBuffer();
	qint64 getAvailable() const {return m_available;} // se usa esto????

signals:
	void packageReady(package_t* pkg);
	void timeout(void);
//	void crcError(void);
//	void targetReportsRxStatus(int status);
//	void rxInProgress(void);

public slots:
	void startRxTimeout(int time_ms);
	void stopRxTimeout(void);

private slots:
	void handleReadyRead(void);
	void handleTimeout(void);
	void handleError(QSerialPort::SerialPortError error);

private:

	void processRx(void);
	int cmdHasData(uint8_t command);

	QSerialPort *m_serialPort = nullptr;
	SerialPortWriter *m_serialPortWriter = nullptr;
	QTextStream m_standardOutput;
	QTimer m_timer;

	int m_state = 0;
	package_t m_pkg;
	QByteArray m_readData;
	QByteArray m_pkgData;
	qint64 m_received = 0;
	qint64 m_available = 0; // se usa??
};

#endif // SERIALPORTREADER_H
