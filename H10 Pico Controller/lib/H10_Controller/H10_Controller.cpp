#include "H10_Controller.h"

H10_Controller* H10_Controller::_activeInstance = nullptr;

H10_Controller::H10_Controller(	uint8_t punchStart,	uint8_t punchReady,	uint8_t readerReady, uint8_t readerStart,
								uint8_t readDataLoad, uint8_t punchDataLatch) :
		_punchStart(punchStart),
		_punchReady(punchReady),
		_readerReady(readerReady),
		_readerStart(readerStart),
		_readDataLoad(readDataLoad),
		_punchDataLatch(punchDataLatch)
{
}

void H10_Controller::begin()
{
	pinMode(_punchStart,    OUTPUT);
	pinMode(_punchReady,    INPUT);
	pinMode(_readerReady,   INPUT);
	pinMode(_readerStart,   OUTPUT);
	pinMode(_readDataLoad,  OUTPUT);
	pinMode(_punchDataLatch, OUTPUT);

	digitalWrite(_punchStart,    LOW);
	digitalWrite(_readerStart,   LOW);
	digitalWrite(_readDataLoad,  HIGH);
	digitalWrite(_punchDataLatch, HIGH);

	// Route punch-ready and reader-ready rising edges to this controller instance.
	_activeInstance = this;
	attachInterrupt(digitalPinToInterrupt(_punchReady),  H10_Controller::onPunchReadyISR,  RISING);
	attachInterrupt(digitalPinToInterrupt(_readerReady), H10_Controller::onReaderReadyISR, RISING);
}

// ---------- Reader ----------

bool H10_Controller::isReaderReady()
{
	return digitalRead(_readerReady) == HIGH;
}

uint8_t H10_Controller::readByte()
{
	// Assert ReaderStart to advance the tape one frame
	digitalWrite(_readerStart, HIGH);
	delayMicroseconds(1);
	digitalWrite(_readerStart, LOW);

	// Wait for ReaderReady
	while (!isReaderReady());

	// Latch the parallel data into the shift register
	digitalWrite(_readDataLoad, LOW);
	delayMicroseconds(1);
	digitalWrite(_readDataLoad, HIGH);

	// Clock the byte out via SPI
	return SPI.transfer(0x00);
}

// ---------- Punch ----------

bool H10_Controller::isPunchReady()
{
	return digitalRead(_punchReady) == HIGH;
}

void H10_Controller::punchByte(uint8_t data)
{
	// Shift data into the latch via SPI
	SPI.transfer(data);

	// Strobe PunchDataLatch to load data into the punch solenoids
	digitalWrite(_punchDataLatch, LOW);
	delayMicroseconds(1);
	digitalWrite(_punchDataLatch, HIGH);

	// Trigger the punch cycle
	digitalWrite(_punchStart, HIGH);
	delayMicroseconds(1);
	digitalWrite(_punchStart, LOW);
}

// Called from ISR context when _punchReady rises.
// Punches the next byte from _punchBuf if one is available.
void H10_Controller::onPunchReadyISR()
{
	if (_activeInstance != nullptr)
	{
		_activeInstance->onPunchReady();
	}
}

void H10_Controller::onPunchReady()
{
	uint8_t data = 0;
	if (_punchBuf.pop(data))
	{
		punchByte(data);
	}
}

// Called from ISR context when _readerReady rises.
// Latches the parallel data, clocks it out via SPI, and stores it in _readerBuf.
// If _readerBuf is full, the reader-ready interrupt is disabled until space is available.
void H10_Controller::onReaderReadyISR()
{
	if (_activeInstance != nullptr)
	{
		_activeInstance->onReaderReady();
	}
}

void H10_Controller::onReaderReady()
{
	// Latch the parallel data into the shift register
	digitalWrite(_readDataLoad, LOW);
	digitalWrite(_readDataLoad, HIGH);

	// Clock the byte out via SPI
	uint8_t data = SPI.transfer(0x00);

	if (!_readerBuf.push(data))
	{
		// Buffer is full – disable this interrupt until the caller drains the buffer
		detachInterrupt(digitalPinToInterrupt(_readerReady));
	}
}


