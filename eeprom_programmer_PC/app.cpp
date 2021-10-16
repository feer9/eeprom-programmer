#include "app.h"


App::App(int &argc, char **argv) :
	QCoreApplication(argc, argv),
	m_standardOutput(stdout),
	m_serialPortWriter(&m_serialPort),
	m_serialPortReader(&m_serialPort)
{
	bool start = configure();
	if(!start) {
		// We can't call exit() before exec() ...
		QTimer::singleShot(0, qApp, SLOT(quit()));
		return;
	}

	if (!m_serialPort.open(QIODevice::ReadWrite)) {

		m_standardOutput << QObject::tr("Failed to open port %1, error: %2")
						  .arg(m_serialPortOptions.name, m_serialPort.errorString())
						 << Qt::endl;

		QTimer::singleShot(0, qApp, SLOT(quit()));
		return;
	}

	m_standardOutput << QObject::tr("Connected to port %1")
					  .arg(m_serialPortOptions.name)
				   << Qt::endl;

	connect(&m_serialPortReader, &SerialPortReader::CallXferHandler,
					 this, &App::handleXfer);

	connect(&m_serialPortReader, &SerialPortReader::CallXferTimedOut,
					 this, &App::handleRxTimedOut);

	connect(&m_timer, &QTimer::timeout,
					 this, &App::handleTimer);


	m_timer.setSingleShot(false);
	m_timer.start(500);
}


void App::setCommandLineOptions(QCommandLineParser& parser)
{
	App::setApplicationName("EEPROM Tool");
	App::setApplicationVersion("1.0");
	parser.setApplicationDescription("Read and write from supported EEPROM memories.");

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
							"Connect to the serial port <port>.", "port", SERIALPORTNAME},
			{{"b", "baudrate"},
							"Set the desired baudrate.", "baudrate"},
		});

	parser.addPositionalArgument("target", "24LC16 - X24645 - 24LC64 - 24LC256");
}

void App::setSerialPortOptions(SerialPortOptions& op)
{
	m_serialPort.setPortName(op.name);
	m_serialPort.setBaudRate(op.baudrate);
	m_serialPort.setDataBits(op.databits);
	m_serialPort.setParity(op.parity);
	m_serialPort.setStopBits(op.stopbits);
	m_serialPort.setFlowControl(op.flowcontrol);
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
		m_standardOutput << "You must select the memory target." << Qt::endl;
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
		setOperation(cmd_write);
		if(!targetFile.isNull())
			setInputFile(targetFile);
	}
	else if(parser.isSet("read")) {
		setOperation(cmd_read);
		if(!targetFile.isNull())
			setOutputFile(targetFile);
	}

	if(parser.isSet("baudrate"))
		m_serialPortOptions.baudrate = parser.value("baudrate").toInt();

/*	QTextStream out(stdout);
	out << "Serial port: " << m_serialPortOptions.name << Qt::endl;
	out << "Baudrate: " << m_serialPortOptions.baudrate << Qt::endl;
	out << "target file: " << targetFile << Qt::endl;
	out << "input file: " << getInputFilename() << Qt::endl;
	out << "output file: " << getOutputFilename() << Qt::endl;
	out << "target device: " << target << Qt::endl;
*/

	setSerialPortOptions(m_serialPortOptions);

	return true;
}

App::~App() {
	if(m_serialPort.isOpen()) {
		sendCommand(cmd_disconnect);
		m_serialPort.waitForBytesWritten(500);
		m_serialPort.close();
	}
}

void App::handleTimer(void) {
	handleXfer(m_serialPortReader.getReadData());
}

void App::handleRxTimedOut() {

	m_areConnected = false;

	m_xferState=0;

	if(m_txrx == MODE_RX) {
		m_standardOutput << "Reading from memory timed out." << Qt::endl;
	}
	else if(m_txrx == MODE_TX) {
		m_standardOutput << "Writing to memory timed out." << Qt::endl;
	}
	else {
		m_standardOutput << "RX timed out." << Qt::endl;
	}
}

int App::cmdHasData(uint8_t command) {
	switch(command) {

	case cmd_init: return 0;

	case cmd_readmem16:  return getMemSize(MEMTYPE_24LC16);
	case cmd_readmemx64: return getMemSize(MEMTYPE_X24645);
	case cmd_readmem256: return getMemSize(MEMTYPE_24LC256);

	case cmd_writemem16:
	case cmd_writememx64:
	case cmd_writemem256: return 0;

	case cmd_ok:  return 0;
	case cmd_err: return 1;

	default: return 0;
	}
}

int App::parsePackage(QByteArray& raw, package_t *pkg) {

	int status = 1;
	pkg->cmd = 0x00;
	pkg->data = NULL;
	pkg->datalen = 0;

	if(raw.length() >= PKG_MINSIZE)
	{
		// check for start transmission block
		status = memcmp(raw.data(), cmd_startxfer, 3);

		if(status == 0)
		{
			pkg->cmd = raw[3];
			raw.remove(0, 4);

			pkg->datalen = cmdHasData(pkg->cmd);

			if(pkg->datalen > 0)
			{
				uint8_t tmp = pkg->cmd & 0xF0;
				if(tmp == cmd_read && m_txrx == MODE_RX &&
						raw.length() >= m_memsize+3) {
					// receiving memory content

					m_memBuffer = raw.left(m_memsize);
					raw.remove(0, m_memsize);
					status = 0;
				}
				else if(pkg->cmd == cmd_err) {
					m_memBuffer = raw.left(1);
					raw.remove(0, 1);
					status = 0; // error but still valid package
				}
				pkg->data = (uint8_t*)(m_memBuffer.data());
			}
			if(status == 0)
			{
				// check for end transmission block
				status = memcmp(raw.data(), cmd_endxfer, 3);
				raw.remove(0, 3);
			}
		}
	}
	if(status != 0)
		raw.clear();

	return status;
}

const QString &App::getOutputFilename() const
{
	return m_filename_out;
}

const QString &App::getInputFilename() const
{
	return m_filename_in;
}

void App::setInputFile(const QString &newFilename_in)
{
	m_filename_in = newFilename_in;
}

void App::setOutputFile(const QString &newFilename_out)
{
	m_filename_out = newFilename_out;
}

void App::setOperation(int newOperation)
{
	m_operation = newOperation;
}

qint64 App::getMemSize(void) {
	return getMemSize(m_memtype);
}

qint64 App::getMemSize(memtype_e type) {
	switch(type) {
	case MEMTYPE_24LC16:  return 0x800;
//	case MEMTYPE_24LC64:
	case MEMTYPE_X24645:  return 0x2000;
	case MEMTYPE_24LC256: return 0x8000;
	default: return -1;
	}
}

void App::reconnect()
{
	m_standardOutput << "Couldn't establish connection with uC." << Qt::endl;
	m_xferState = 0;
	m_areConnected = false;
	m_serialPort.close();
	if (!m_serialPort.open(QIODevice::ReadWrite)) {
		m_standardOutput << "Unable to open serial port. Aborting." << Qt::endl;
		QCoreApplication::exit(1);
	}
}

// this method is NASTY. You have been advised
void App::handleXfer(QByteArray& rData) {

	qint64 available = rData.length();
	package_t pkg;
	static unsigned int retries=0;

	switch(m_xferState)
	{
	case 0: // trying to connect

		sendCommand(cmd_init);
		m_xferState = 1;
		break;


	case 1: // Waiting for INIT response

		if(available >= PKG_MINSIZE) // start(3) + cmd(1) + end(3)
		{
			retries=0;

			if(parsePackage(rData, &pkg) != 0) {
				reconnect();
				break;
			}

			if(pkg.cmd == cmd_init) {

				m_standardOutput << "Connected to uC." << Qt::endl;

				m_xferState++;
				m_areConnected = true;
			}
			else {
				m_standardOutput << "Could not connect to uC" << Qt::endl;
				clearBuffers();
				m_xferState = 0;
			}
		} else if (++retries == 10){
			retries=0;
			reconnect();
		}
		break;

//		[[fallthrough]];
	case 2: //
		if(!m_areConnected) {
			m_xferState = 0;
			clearBuffers();
			break;
		}
		if(m_operation != 0) {
			m_xferState = 4;
		}
		else {
			sendcmd_ping();
			m_xferState = 3;
		}

		break;

	case 3: // receive ping
		if(available >= PKG_MINSIZE)
		{
			if(parsePackage(rData, &pkg) == 0 &&
					pkg.cmd == cmd_ping) {
				m_standardOutput << "ping." << Qt::endl;
				m_xferState = 2;
			}
			else {
				m_areConnected = false;
				m_xferState = 0;
			}
		}
		break;

	case 4:

		if(m_operation == cmd_read)
			sendcmd_readMem();

		else if(m_operation == cmd_write)
			sendcmd_writeMem();

		else
			m_xferState = 2;

		break;

	case 10: // requested read eeprom - waiting memory data

		if(available < PKG_MINSIZE + m_memsize) {
			break;
		}

		if(++m_trys > 10000) { // adjust this
			m_xferState = 0;
			m_areConnected = false;
			m_trys = 0;
			m_standardOutput << "uC connection timed out." << Qt::endl;
			break;
		}

#ifdef DEBUG
		m_standardOutput << "State 10. Bytes available: " << available << Qt::endl;
		m_standardOutput << rData.toHex() << Qt::endl;
#endif

		if(parsePackage(rData, &pkg) != 0) {
			reconnect();
			break;
		}

		if(pkg.cmd == getCommandByte(m_memtype, m_txrx)) {
			// finished receiving memory data
			printData();
			saveData();
			m_xferState=2;
			QCoreApplication::exit(0); // TODO: temporary
		}
		else if(pkg.cmd == cmd_err) {
			m_standardOutput << "uC ERROR " << int(pkg.data[0]) << " reading data..." << Qt::endl;
			m_xferState = 2;
		}
		else {
			m_standardOutput << "Unknown error from uC" << Qt::endl;
			reconnect();
		}
		m_trys = 0;
		clearBuffers();

		break;


	case 20: // requested eeprom full write - waiting confirmation

		if(available < PKG_MINSIZE)
			break;

		if(parsePackage(rData, &pkg) != 0) {
			reconnect();
			break;
		}

		if(pkg.cmd == cmd_ok) {
			m_standardOutput << "Memory write SUCCESSFULLY" << Qt::endl;
			m_xferState=2;
		}
		else if(pkg.cmd == cmd_err) {
			m_standardOutput << "uC ERROR " << int(pkg.data[0]) << " writing data..." << Qt::endl;
			clearBuffers();
			m_xferState=2;
		}
		else {
			clearBuffers();
			if(++m_trys >= 20) {
				reconnect();
				m_trys = 0;
			}
			else {
				m_xferState = 2;
			}
		}

		QCoreApplication::exit(0); // TODO: temporary
		break;

	default:
		while(1); // catch the bug :-)
	}
}
// TODO: split into simple methods


void App::clearBuffers() {
	m_serialPortReader.clearBuffer();
	m_memBuffer.clear();
}

void App::setMem_X24645() {
	setTargetMem(MEMTYPE_X24645);
}
void App::setMem_24LC16() {
	setTargetMem(MEMTYPE_24LC16);
}
void App::setMem_24LC256() {
	setTargetMem(MEMTYPE_24LC256);
}
bool App::setTargetMem(memtype_e memtype) {
	if(memtype < MEMTYPE_24LC16 || memtype > MEMTYPE_mAX)
		return false;
	m_memtype = memtype;
	m_memsize = getMemSize(memtype);
	return true;
}
bool App::setTargetMem(QString memtype) {

	QString Memtype = memtype.toUpper();

	if(Memtype == "24LC16")
		m_memtype = MEMTYPE_24LC16;
	else if (Memtype == "X24645")
		m_memtype = MEMTYPE_X24645;
//	else if (Memtype == "24LC64")
//		m_memtype = MEMTYPE_24LC64;
	else if (Memtype == "24LC256")
		m_memtype = MEMTYPE_24LC256;
	else {
		return false;
	}

	m_memsize = getMemSize(m_memtype);
	return true;
}


void App::sendCommand(uint8_t cmd) {
	m_currCmd = cmd;
	m_serialPortWriter.sendPackage(&m_currCmd);
}

void App::sendCommand(uint8_t cmd, const QByteArray& data) {
	m_currCmd = cmd;
	m_serialPortWriter.sendPackage(&m_currCmd, data);
}

void App::sendcmd_readMem() {

	m_txrx = MODE_RX;
	m_memBuffer.clear();
	m_memBuffer.resize(m_memsize);
	m_xferState = 10;

	sendCommand(getCommandByte(m_memtype, MODE_RX));
}

void App::sendcmd_writeMem() {

	QFile file(m_filename_in);

	if (!file.open(QIODevice::ReadOnly)) {
		m_standardOutput << "Could not open file \"" << m_filename_in << "\"" << Qt::endl;
		return;
	}

	m_memBuffer.clear();
	m_memBuffer.resize(m_memsize);
	m_memBuffer = file.readAll();
	file.close();

	if(m_memBuffer.length() != m_memsize) {
		m_standardOutput << "File size don't match." << Qt::endl;
		m_xferState = 2;
		return;
	}

	m_txrx = MODE_TX;
	m_xferState=20;

	m_serialPortReader.clearBuffer();
	m_serialPort.clear(QSerialPort::Input);

	sendCommand(getCommandByte(m_memtype, m_txrx), m_memBuffer);

	m_timer.start(5000); // set timeout
}

void App::sendcmd_ping(void) {
	sendCommand(cmd_ping);
}

uint8_t App::getCommandByte(int memtype, int txrx) {
	uint8_t cmd;

	if(txrx == MODE_RX)
		cmd = 0x40;
	else if(txrx == MODE_TX)
		cmd = 0x50;
	else return 0;

	cmd |= memtype;
	return cmd;
}


void App::printData() {

//	m_standardOutput << m_readData.toHex() << Qt::endl;
	int i;
	char buf[128];
//	QByteArray buff[128];
//	QByteArray::Iterator it = buff->begin();
	uint8_t *page = (uint8_t*)(m_memBuffer.data());

	for(int reg=0; reg < m_memsize; reg += 32)
	{
		char *p = buf;
		sprintf(p, "%04X: ", reg );
		p+=6;
		for(i=0; i<16; ++i) {
			sprintf(p+3*i, "%02X ", int(page[reg+i]) );
		}
		m_standardOutput << buf << Qt::endl;

		p=buf;
		sprintf(p, "%04X: ", reg+16 );
		p+=6;
		for(i=0; i<16; ++i) {
			sprintf(p+3*i, "%02X ", int(page[16+reg+i]) );
		}
		m_standardOutput << buf << Qt::endl;
	}
	// No time for doing better
}


void App::saveData() {

	QFile file(m_filename_out);

	if (!file.open(QIODevice::WriteOnly)) {
		m_standardOutput << "Could not open file \"" << m_filename_out << "\"" << Qt::endl;
		return;
	}

	file.write(m_memBuffer);
	file.close();
	m_standardOutput << "Saved data to file \"" << m_filename_out << "\"" << Qt::endl;
}
