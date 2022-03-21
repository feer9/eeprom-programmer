#ifndef SERIALPORTWRITER_H
#define SERIALPORTWRITER_H

#include <QByteArray>
#include <QObject>
#include <QSerialPort>
#include <QTextStream>
#include <QTimer>
#include <QCoreApplication>
#include <QDebug>

#include "eeprom.h"
#include "crc16.h"

class SerialPortReader;

class SerialPortWriter : public QObject
{
	Q_OBJECT
public:
	explicit SerialPortWriter(QSerialPort *serialPort,
							  FILE* outStream,
							  QObject *parent = nullptr);

	virtual ~SerialPortWriter() {qDebug() << "Data sent:     " << m_totalBytesSent; };

	qint64 send(commands_e cmd);
	qint64 send(commands_e cmd, const QByteArray &data);
	inline qint64 getBytesSent() const {return m_totalBytesSent;};

	bool busy(void) const;

signals:
//	void txXferComplete(int status);
	void packageSent(commands_e);

public slots:
//	void handleTargetReportRxStatus(int status);
//	void sendRxAck(void);

private slots:
	void handleBytesWritten(qint64 bytes);
	void handleTimeout(void);
	void handleError(QSerialPort::SerialPortError error);

private:
	void finishTxXfer(int status);
	qint64 write(void);
	qint64 sendPackage(void);

	QTextStream m_standardOutput;
	QSerialPort *m_serialPort = nullptr;
	SerialPortReader *m_serialPortReader = nullptr;
	QTimer m_timer;

	QByteArray m_package;				/* current package with command, data, chksum... */
	QByteArray m_packageData;			/* current package data */
	QByteArray m_data;					/* whole message data */
	commands_e m_cmd;
	qint64 m_totalBytesSent = 0;		/* Total number of bytes sent in this session */
	qint64 m_packageBytesWritten = 0;	/* Bytes sent in the current transfer */
	qint64 m_bytesRemaining = 0;		/* data bytes remaining from current transfer*/
	bool m_busy = false;
	int m_tries = 0;
	void transmitNextPackage();
	void targetRxError();
	void setSignals();
};

#endif // SERIALPORTWRITER_H
