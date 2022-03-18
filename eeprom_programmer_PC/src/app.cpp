#include "app.h"



App::App(int &argc, char **argv, FILE* outStream)
	: MemoryComm(argc, argv, outStream)
{
	bool start = configure();
	if(!start) {
		// We can't call exit() before exec() ...
		// ToDo: exit with error code
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
	App::setApplicationVersion("1.0");
	parser.setApplicationDescription("Read and write EEPROM memories.");

	parser.addHelpOption();
	parser.addVersionOption();

	parser.addOptions({
			{{"r", "read"},
							"Read memory content."},
			{{"w", "write"},
							"Write memory content."},
			{{"f", "file"},
							"Read from / write to <file>.", "file"},
			{{"p", "port"},
							"Select the serial port to connect.", "port", SERIALPORTNAME},
			{{"b", "baudrate"},
							"Set the desired baudrate.", "baudrate"},
		});

	parser.addPositionalArgument("target", "24LC16 - X24645 - 24LC64 - 24LC256");
}

bool App::configure() {

	QCommandLineParser parser;
	setCommandLineOptions(parser);

	parser.process(*this);


	const QStringList args = parser.positionalArguments();
	const QString target = args.isEmpty() ? QString() : args.first();

	if(parser.isSet("h") || parser.isSet("v")) {
		 return false;
	}
	if(args.size() < 1) {
		m_standardOutput << "Error: you must select the memory target." << Qt::endl;
		m_standardOutput << parser.helpText() << Qt::endl;
		return false;
	}
	if(!setTargetMem(target)) {
		m_standardOutput << "Error: invalid memory type selected." << Qt::endl;
		m_standardOutput << parser.helpText() << Qt::endl;
		return false;
	}

	const QString targetFile = parser.value("file");
	m_serialPortOptions.name = parser.value("port");

	if(parser.isSet("write")) {
		setOperation(CMD_WRITEMEM);
		if(!targetFile.isNull())
			setInputFilename(targetFile);
	}
	else if(parser.isSet("read")) {
		setOperation(CMD_READMEM);
		if(!targetFile.isNull())
			setOutputFilename(targetFile);
	}

	if(parser.isSet("baudrate"))
		m_serialPortOptions.baudrate = parser.value("baudrate").toInt();

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

void App::setOperation(int newOperation)
{
	m_operation = newOperation;
}
