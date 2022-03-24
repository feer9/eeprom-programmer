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
	};

	enum operations_e {
		OP_NONE = CMD_NONE,
		OP_PING = CMD_PING,
		OP_TX = CMD_WRITEMEM,
		OP_RX = CMD_READMEM
	};

	enum comm_states_e {
		COMM_IDLE,
		COMM_READMEM_WAIT_OK,
		COMM_READMEM_WAIT_DATA,
		COMM_WRITEMEM_WAIT_OK,
		COMM_WRITEMEM_WAIT_ACK
	};

protected:
	void setSerialPortOptions(SerialPortOptions& op);

	bool writeMem(const QByteArray& memBuffer);
	bool readMem(void);
	bool sendCommand_ping(void);
	bool sendCommand_memid(void);
	bool sendCommand(commands_e cmd);
	bool sendCommand(commands_e cmd, uint8_t data);
	bool sendCommand(commands_e cmd, const QByteArray& data);

	void clearBuffers(void);

	void reconnect(void);
	virtual void handleXfer(pkgdata_t *pkg) = 0;

private:
	void setSignals();
	void packageReady(package_t *pkg);
	bool sendMemoryBlock();
	void setPackageError(package_t *pkg, errorcode_e err);

signals:

private slots:
	void handlePackageReceived(package_t *pkg);
	void setRxTimeout(commands_e);
	void handleRxTimedOut(void);

	void handleRxCrcError(void);

	void handlePackageSent(commands_e cmd);
//	void handleTxXferComplete(int status);


	// Member variables definitions:
private:
	commands_e m_lastTxCmd = CMD_NONE;
	commands_e m_lastRxCmd = CMD_NONE;
	QByteArray m_buffer;
	QByteArray m_memBuffer;
	int m_memindex = 0;
	pkgdata_t m_pkg;

	operations_e m_operation = OP_NONE;
	comm_states_e m_commState = COMM_IDLE;

	QByteArray m_pending;

	void errorReceived(package_t *pkg);

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
