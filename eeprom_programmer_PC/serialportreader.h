#ifndef SERIALPORTREADER_H
#define SERIALPORTREADER_H

#include <QObject>
#include <QCoreApplication>
#include <QByteArray>
#include <QSerialPort>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QDateTime>
#include <QFile>



class SerialPortReader : public QObject
{
	Q_OBJECT

protected:

public:
	SerialPortReader(QSerialPort *serialPort, QObject *parent = nullptr);
	virtual ~SerialPortReader() {m_standardOutput << "Datos recibidos: "
												  << getRecibidos() << Qt::endl; }

	inline qint64 getRecibidos() const {return m_recibidos;}
	void clearBuffer();
	void cleanBuffer();
	qint64 getAvailable() const {return m_available;}
	QByteArray& getReadData() {return m_readData;}

signals:
	void CallXferHandler(QByteArray&);
	void CallXferTimedOut(void);

public slots:

private slots:
	void handleReadyRead(void);
	void handleTimeout(void);
	void handleError(QSerialPort::SerialPortError error);

private:
	QSerialPort *m_serialPort = nullptr;
	QByteArray m_readData;
	QTextStream m_standardOutput;
	QTimer m_timer;
	qint64 m_recibidos = 0;
	qint64 m_available = 0;
};

#endif // SERIALPORTREADER_H
