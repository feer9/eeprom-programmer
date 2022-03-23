/*
 *  EEPROM-Programmer - Read and write EEPROM memories.
 *  Copyright (C) 2022  Fernando Coda <fcoda@pm.me>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "app.h"


App::App(int &argc, char **argv, FILE* outStream)
	: MemoryComm(argc, argv, outStream)
{
	bool start = configure();
	if(!start) {
		// We can't call exit() before exec() ...
		QTimer::singleShot(0, qApp, SLOT(quit()));
		return;
	}

	if (!m_serialPort.open(QIODevice::ReadWrite)) {

		m_standardOutput << QObject::tr("Failed to open port %1: %2")
						  .arg(m_serialPortOptions.name, m_serialPort.errorString())
						 << Qt::endl;

		QTimer::singleShot(0, qApp, SLOT(quit()));
		return;
	}

	m_standardOutput << QObject::tr("Connected to port %1")
					  .arg(m_serialPortOptions.name)
				   << Qt::endl;

	setSignals();

	m_pingTimer.setSingleShot(false);
	m_pingTimer.start(500);

	// Start communication
	App::handleXfer(nullptr);
}

void App::setSignals()
{
	connect(&m_pingTimer, &QTimer::timeout,
					this, &App::pingTimerLoop);

	connect(&m_serialPortReader, &SerialPortReader::timeout,
						   this, &App::handleTimeout);
}

void App::setCommandLineOptions(QCommandLineParser& parser)
{
	App::setApplicationName("EEPROM Programmer");
	App::setApplicationVersion("2.0-rc1");
	parser.setApplicationDescription("Read and write EEPROM memories.");

	parser.addOption({{"h", "help"},
					  "Displays help on commandline options."});
	parser.addVersionOption();

	parser.addOptions({
			{{"r", "read"},
							"Read memory content."},
			{{"w", "write"},
							"Write memory content."},
			{{"f", "file"},
							"Read from / write to <file>.", "file"},
			{{"p", "port"},
							"Connect to serial port <port>.", "port"},
			{{"b", "baudrate"},
							"Set the serial port baudrate to <baudrate>.", "baudrate"},
		});

	parser.addPositionalArgument("target", "24LC16 - X24645 - 24LC64 - 24LC256");
}

bool App::configure() {

	QCommandLineParser parser;
	setCommandLineOptions(parser);

	parser.process(*this);


	const QStringList args = parser.positionalArguments();
	const QString target = args.isEmpty() ? QString() : args.first();

	if(parser.isSet("h")) {
		parser.showHelp(0);
		return false;
	}
	if(args.size() < 1) {
		m_standardOutput << "Error: you must select the memory target." << Qt::endl;
		parser.showHelp(1);
		return false;
	}
	if(!setTargetMem(target)) {
		m_standardOutput << "Error: invalid memory type selected." << Qt::endl;
		parser.showHelp(1);
		return false;
	}

	const QString targetFile = parser.value("file");

	if(parser.isSet("port")) {
		m_serialPortOptions.name = parser.value("port");
	}

	if(parser.isSet("write")) {
		setNextOperation(OP_TX);
		if(!targetFile.isNull())
			setInputFilename(targetFile);
	}
	else if(parser.isSet("read")) {
		setNextOperation(OP_RX);
		if(!targetFile.isNull())
			setOutputFilename(targetFile);
	}

	if(parser.isSet("baudrate")) {
		m_serialPortOptions.baudrate = parser.value("baudrate").toInt();
	}

	qDebug() << "Serial port:   " << m_serialPortOptions.name;
	qDebug() << "Baudrate:      " << m_serialPortOptions.baudrate;
	qDebug() << "Target file:   " << targetFile;
	qDebug() << "Input  file:   " << getInputFilename();
	qDebug() << "Output file:   " << getOutputFilename();
	qDebug() << "Target device: " << target;

	setSerialPortOptions(m_serialPortOptions);

	return true;
}

App::~App() {

}

const QString &App::getOutputFilename() const
{
	return m_filename_out;
}

const QString &App::getInputFilename() const
{
	return m_filename_in;
}

void App::setInputFilename(const QString &newFilename_in)
{
	m_filename_in = newFilename_in;
}

void App::setOutputFilename(const QString &newFilename_out)
{
	m_filename_out = newFilename_out;
}

void App::setNextOperation(operations_e newOperation)
{
	m_nextOperation = newOperation;
}
