#ifndef SERIALPORTWRITER_H
#define SERIALPORTWRITER_H

#include <QByteArray>
#include <QObject>
#include <QSerialPort>
#include <QTextStream>
#include <QTimer>
#include <QCoreApplication>


#include "commands.h"

class SerialPortWriter : public QObject
{
	Q_OBJECT
public:
	explicit SerialPortWriter(QSerialPort *serialPort, QObject *parent = nullptr);
	virtual ~SerialPortWriter() {m_standardOutput << "Data sent: "
												  << enviados << Qt::endl; }
	void write(const QByteArray &writeData);
	void write(const char *writeData, qint64 len);
	void sendPackage(const char *cmd, const QByteArray &data);
	void sendPackage(const char *cmd);
	inline qint64 getEnviados() {return enviados;}

signals:

public slots:

private slots:
	void handleBytesWritten(qint64 bytes);
	void handleTimeout();
	void handleError(QSerialPort::SerialPortError error);

private:
	QSerialPort *m_serialPort = nullptr;
	QByteArray m_writeData;
	QTextStream m_standardOutput;
	QTimer m_timer;
	qint64 m_bytesWritten = 0;
	qint64 enviados = 0;
};

#endif // SERIALPORTWRITER_H
