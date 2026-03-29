// ComUART.cpp - Implementation of UART-base serial port
//
// Receiving
//  - setEcho() is used to enable echo of received characters
//	- Uses Arduino serialEventx to proccess incoming characters
//	- Received characters are put into a line buffer until LF is received
//	- CR characters are ignored
//	- hasLine() returns true if a full line is available
//  - While hasLine() is true, incomming bytes are not processed
//	- lineClear() or readLine() methods re-enables character processing
//
// Transmission
//	- 

#include "ComUART.h"

ComUART::ComUART(HardwareSerialIMXRT& serialPort, uint8_t rtsPin, uint8_t ctsPin)	:
	_serialPort(serialPort),
	_rtsPin(rtsPin),
	_ctsPin(ctsPin),
	_lineLength(0),
	_lineReady(false),
	_overflow(false),
	_echo(true)
{
}

void ComUART::begin(uint32_t baudRate, uint16_t config)
{
	_serialPort.begin(baudRate, config);
	//_serialPort.attachRts(_rtsPin);
	//_serialPort.attachCts(_ctsPin);
	_lineLength = 0;
	_lineReady = false;
	_overflow = false;
	_serialPort.addMemoryForWrite(_txBuffer, 144);
	_serialPort.addMemoryForRead(_rxBuffer, 144);
}

void ComUART::setEcho(bool enabled)
{
	_echo = enabled;
}

// Rx processing

bool ComUART::hasLine() const
{
	return _lineReady;
}

size_t ComUART::readLine(char* outBuffer, size_t outBufferSize)
{
	digitalWrite(LED1, 0);
	if ((outBuffer == nullptr) || (outBufferSize == 0))
	{
		return 0;
	}

	if (!_lineReady)
	{
		outBuffer[0] = '\0';
		return 0;
	}

	size_t copyLen = _lineLength;
	if (copyLen >= outBufferSize)
	{
		copyLen = outBufferSize - 1;
	}

	for (size_t i = 0; i < copyLen; ++i)
	{
		outBuffer[i] = _lineBuffer[i];
	}
	outBuffer[copyLen] = '\0';

	_lineLength = 0;
	_lineReady = false;
	_overflow = false;

	return copyLen;
}

bool ComUART::overflowed() const
{
	return _overflow;
}

void ComUART::clearLine()
{
	digitalWrite(LED1, 0);
	noInterrupts();
	_lineLength = 0;
	_lineBuffer[0] = '\0';
	_lineReady = false;
	_overflow = false;
	interrupts();
}

void ComUART::serviceRx()
{
	while (_serialPort.available() > 0)
	{
		char ch = static_cast<char>(_serialPort.read());
		if (_echo)
			_serialPort.write(static_cast<uint8_t>(ch));

		if (_lineReady)
			return;

		if (ch == '\r')
			continue;

		if (ch == '\n')
		{
			_lineReady = true;
			digitalWrite(LED1, 1);
			return;
		}

		if (_lineLength >= (sizeof(_lineBuffer) - 1))
		{
			_overflow = true;
			return;
		}

		_lineBuffer[_lineLength] = ch;	 // add char to line buffer
		_lineLength++;
		_lineBuffer[_lineLength] = '\0'; // add null terminator
	}
}


// Tx processing

size_t ComUART::write(uint8_t data)
{
	return _serialPort.write(data);
}

size_t ComUART::writeLine(const char* text)
{
	if (text == nullptr)
		return 0;

	const size_t written = _serialPort.print(text);
	_serialPort.print("\r\n");
	return written;
}

