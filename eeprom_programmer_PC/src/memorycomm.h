#ifndef MEMORYCOMM_H
#define MEMORYCOMM_H

#include "eeprom.h"
#include "serialportreader.h"
#include "serialportwriter.h"
#include <QSerialPort>
#include <QCoreApplication>

#ifdef _WIN32
#define SERIALPORTNAME "COM0"
#else
#define SERIALPORTNAME "ttyACM0"
#endif


enum modes_e {
	MODE_NONE,
	MODE_TX,
	MODE_RX
};


class MemoryComm : public QCoreApplication, public EEPROM
{
	Q_OBJECT
public:
	explicit MemoryComm(int &argc, char **argv, FILE* outStream);
	~MemoryComm();

	SerialPortWriter& getSerialPortWriter(void) {return m_serialPortWriter;};
	SerialPortReader& getSerialPortReader(void) {return m_serialPortReader;};

	struct SerialPortOptions {
		QString name							= SERIALPORTNAME;
		qint32 baudrate							= QSerialPort::Baud115200;
		QSerialPort::DataBits databits			= QSerialPort::Data8;
		QSerialPort::Parity parity				= QSerialPort::NoParity;
		QSerialPort::StopBits stopbits			= QSerialPort::OneStop;
		QSerialPort::FlowControl flowcontrol	= QSerialPort::NoFlowControl;
	} ;

protected:
	void setSerialPortOptions(SerialPortOptions& op);

	bool writeMem(const QByteArray& memBuffer);
	void readMem(void);
	void sendCommand_ping(void);
	void sendCommand(commands_e cmd);
	void sendCommand(commands_e cmd, uint8_t data);
	void sendCommand(commands_e cmd, const QByteArray& data);

	void clearBuffers(void);

	void reconnect(void);
	virtual void handleXfer(pkgdata_t *pkg) = 0;

private:
	void setSignals();
	void packageReady(package_t *pkg);

signals:

public slots:
	void handlePackageReceived(package_t *pkg);
	void handlePackageSent(commands_e);
	void handleRxTimedOut(void);

	void handleRxCrcError(void);

	void handleTxXferComplete(int status);


	// Member variables definitions:
private:
	commands_e m_lastTxCmd = CMD_NONE;
	commands_e m_lastRxCmd = CMD_NONE;
	pkgdata_t m_pkg;
	QByteArray m_buffer;

	modes_e m_xferMode = MODE_NONE;
	uint8_t m_xferState = 0;

protected:
	QTextStream m_standardOutput;

	QSerialPort m_serialPort;
	SerialPortOptions m_serialPortOptions;
	// Create Writer and Reader classes as members,
	// since they can't be child classes
	SerialPortWriter m_serialPortWriter;
	SerialPortReader m_serialPortReader;
};

#endif // MEMORYCOMM_H
