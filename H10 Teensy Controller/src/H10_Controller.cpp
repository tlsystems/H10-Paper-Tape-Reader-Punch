#include "H10_Controller.h"
#include "defs.h"
H10_Controller* H10_Controller::_activeInstance = nullptr;

H10_Controller::H10_Controller()
{
	
}

void H10_Controller::begin()
{
	// initialize SPI
	SPI.setMISO(MISO);
	SPI.setMOSI(MOSI);
	SPI.setSCK(SCK);
	SPI.begin();

	// initialize shift register control pins
	pinMode(ReadDataLoad, OUTPUT);
	pinMode(PunchDataLatch, OUTPUT);

	// initialize control pins
	pinMode(PunchStart, OUTPUT);
	pinMode(PunchReady, INPUT);
	pinMode(ReaderReady, INPUT);
	pinMode(ReaderStart, OUTPUT);

	digitalWrite(PunchStart,     LOW);
	digitalWrite(ReaderStart,    LOW);
	digitalWrite(ReadDataLoad,   HIGH);
	digitalWrite(PunchDataLatch, HIGH);

	_punchLatchCycles = 0;
	_punchStartCycles = 0;

	// Route punch-ready and reader-ready rising edges to this controller instance.
	_activeInstance = this;
	attachInterrupt(digitalPinToInterrupt(PunchReady),  H10_Controller::onPunchReadyISR,  RISING);
	attachInterrupt(digitalPinToInterrupt(ReaderReady), H10_Controller::onReaderReadyISR, RISING);
}

// ---------- Reader ----------

bool H10_Controller::isReaderReady()
{
	return digitalRead(ReaderReady) == HIGH;
}

uint8_t H10_Controller::readByte()
{
	// Assert ReaderStart to advance the tape one frame
	digitalWrite(ReaderStart, HIGH);
	delayMicroseconds(1);
	digitalWrite(ReaderStart, LOW);

	// Wait for ReaderReady
	while (!isReaderReady());

	// Latch the parallel data into the shift register
	digitalWrite(ReadDataLoad, LOW);
	delayMicroseconds(1);
	digitalWrite(ReadDataLoad, HIGH);

	// Clock the byte out via SPI
	return SPI.transfer(0x00);
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
	digitalWrite(ReadDataLoad, LOW);
	digitalWrite(ReadDataLoad, HIGH);

	// Clock the byte out via SPI
	uint8_t data = SPI.transfer(0x00);

	if (!_readerBuf.push(data))
	{
		// Buffer is full – disable this interrupt until the caller drains the buffer
		detachInterrupt(digitalPinToInterrupt(ReaderReady));
	}
}


// ---------- Punch ----------

bool H10_Controller::isPunchReady()
{
	return digitalRead(PunchReady) == HIGH;
}

bool H10_Controller::queuePunchByte(uint8_t data)
{
	punchByteImmediate(data);
	return true;
	
	const bool wasEmpty = _punchBuf.isEmpty();
	if (!_punchBuf.push(data))
	{
		return false;
	}

	// If the queue was idle and the punch is ready, kick the first cycle now.
	// Otherwise the next punch-ready interrupt will service the queue.
	if (wasEmpty && isPunchReady())
	{
		onPunchReady();
	}

	return true;
}

void H10_Controller::punchByteImmediate(uint8_t data)
{
	// Display bit pattern with each bit duplicated: 'X' for 1 bits, ' ' for 0 bits (MSB first)
	char bitStr[17];
	for (int i = 7; i >= 0; i--)
	{
		char bitChar = (data & (1 << i)) ? 'X' : ' ';
		int idx = (7 - i) * 2;
		bitStr[idx] = bitChar;
		bitStr[idx + 1] = bitChar;
	}
	bitStr[16] = '\0';
	Serial.println(bitStr);
	return;

	// Shift data into the punch data '595
	SPI.transfer(data);

	// pulse _punchDataLatch low to high to trasfer data into '595 outputs 
	cyclePunchLatch(_punchLatchCycles);

	// pulse _punchStart high to low to start a punch cycle
	pulsePunchStart(_punchStartCycles);
}

void H10_Controller::cyclePunchLatch(uint32_t pulseCycles)
{
#if defined(TEENSY)
	(void)pulseCycles;
	noInterrupts();
	digitalWrite(_punchDataLatch, LOW);
	delayMicroseconds(1);
	digitalWrite(_punchDataLatch, HIGH);
	interrupts();
#elif defined(PICO)	
#endif
}

void H10_Controller::pulsePunchStart(uint32_t pulseCycles)
{
	(void)pulseCycles;
	noInterrupts();
	digitalWrite(PunchStart, HIGH);
	delayMicroseconds(1);
	digitalWrite(PunchStart, LOW);
	interrupts();
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
		punchByteImmediate(data);
	}
}


