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


	void setOperation(int newOperation);
	void setOutputFilename(const QString &newFilename_out);
	void setInputFilename(const QString &newFilename_in);

	const QString &getInputFilename() const;
	const QString &getOutputFilename() const;


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

	int m_xferState = 0;
	bool m_connected = false;
	int m_operation = 0;

	QString m_filename_in = "mem_in.bin";
	QString m_filename_out = "mem_out.bin";
	void setSignals();
};

// m_ = member

#endif // APP_H
