#ifndef APP_H
#define APP_H

#include "memorycomm.h"

#include <QByteArray>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QCommandLineParser>
#include <QFile>

class App: public MemoryComm
{

public:
	App(int &argc, char **argv, FILE* outStream = stdout);
	~App();

	void setOutputFilename(const QString &newFilename_out);
	void setInputFilename(const QString &newFilename_in);
	void setNextOperation(operations_e newOperation);

	const QString &getInputFilename() const;
	const QString &getOutputFilename() const;

	enum app_states_e {
		ST_DISCONNECTED,
		ST_INIT,
		ST_IDLE,
		ST_MEMID,
		ST_PING,
		ST_WAIT_READMEM,
		ST_WAIT_WRITEMEM
	};

private slots:
	void handleTimeout(void);
	void pingTimerLoop(void);

private:
	void handleXfer(pkgdata_t *pkg);

	void setCommandLineOptions(QCommandLineParser& parser);
	void printError(pkgdata_t *pkg);
	bool configure(void);

	void printData(void);
	virtual bool saveData(void);
	virtual void reconnect();

	bool writeMem(void);

	QByteArray m_memBuffer;
	QTimer m_pingTimer;

	app_states_e  m_xferState = ST_DISCONNECTED;
	bool m_connected = false;
	// Use two variables so we can change one without affecting
	// the other (new requests will go to m_nextOperation).
	operations_e m_currentOperation = OP_NONE;
	operations_e m_nextOperation    = OP_NONE;

	QString m_filename_in  = "mem_in.bin";
	QString m_filename_out = "mem_out.bin";
	void setSignals();
	bool doSomething();
	void retryConnection();
};

// m_ = member

#endif // APP_H
