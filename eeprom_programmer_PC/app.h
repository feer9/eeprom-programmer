#ifndef APP_H
#define APP_H


#include "serialportreader.h"
#include "serialportwriter.h"

#include <QCoreApplication>
//#include <QApplication>
#include <QByteArray>
#include <QSerialPort>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QCommandLineParser>

#include "commands.h"

#ifdef _WIN32
#define SERIALPORTNAME "COM0"
#else
#define SERIALPORTNAME "ttyUSB0"
#endif


class App : public QCoreApplication
{
	Q_OBJECT

public:
	App(int &argc, char **argv);
	~App();

	struct SerialPortOptions {
		QString name							= SERIALPORTNAME;
		qint32 baudrate							= QSerialPort::Baud115200;
		QSerialPort::DataBits databits			= QSerialPort::Data8;
		QSerialPort::Parity parity				= QSerialPort::NoParity;
		QSerialPort::StopBits stopbits			= QSerialPort::OneStop;
		QSerialPort::FlowControl flowcontrol	= QSerialPort::NoFlowControl;
	} ;

	void setMem_24LC16(void);
	void setMem_X24645(void);
	void setMem_24LC256(void);
	bool setTargetMem(memtype_e memtype);
	bool setTargetMem(QString memtype);

	void setOperation(int newOperation);
	void setOutputFile(const QString &newFilename_out);
	void setInputFile(const QString &newFilename_in);

	memtype_e getMemType(void) {return m_memtype;}
	qint64 getMemSize(void);
	qint64 getMemSize(memtype_e);


	const QString &getInputFilename() const;

	const QString &getOutputFilename() const;

	void setSerialPortOptions(SerialPortOptions& op);

public slots:
	void handleXfer(QByteArray& data);
	void handleRxTimedOut(void);
	void handleTimer(void);

private:


	bool configure(void);
	void clearBuffers(void);
	void sendcmd_writeMem(void);
	void sendcmd_readMem(void);
	void sendcmd_ping(void);

	void printData(void);
	void saveData(void);
	void reconnect();

	int cmdHasData(uint8_t command);
	uint8_t getCommandByte(int,int);
	void sendCommand(uint8_t cmd);
	void sendCommand(uint8_t cmd, const QByteArray& data);
	int parsePackage(QByteArray& data, package_t *pkg);



	QTextStream m_standardOutput;
	QSerialPort m_serialPort;
	SerialPortOptions m_serialPortOptions;
	SerialPortWriter m_serialPortWriter;
	SerialPortReader m_serialPortReader;

	QByteArray m_memBuffer;
	QTimer m_timer;

	int m_xferState = 0;
	bool m_areConnected = false;
	modes_e m_txrx = MODE_NONE;
	memtype_e m_memtype = MEMTYPE_NONE;
	int m_memsize;
	int m_trys = 0;
	char m_currCmd;
	int m_operation = 0;

	QString m_filename_in = "mem_in.bin";
	QString m_filename_out = "mem_out.bin";
	void setCommandLineOptions(QCommandLineParser& parser);
};

#endif // APP_H
