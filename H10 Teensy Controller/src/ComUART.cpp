#include "ComUART.h"

ComUART::ComUART(HardwareSerialIMXRT& serialPort, uint8_t rtsPin, uint8_t ctsPin)	:
	_serialPort(serialPort),
	_rtsPin(rtsPin),
	_ctsPin(ctsPin),
	_lineLength(0),
	_lineReady(false),
	_overflow(false),
	_skipNextLF(false)
{
}

void ComUART::begin(uint32_t baudRate, uint16_t config)
{
	_serialPort.begin(baudRate, config);
	_serialPort.attachRts(_rtsPin);
	_serialPort.attachCts(_ctsPin);
	_lineLength = 0;
	_lineReady = false;
	_overflow = false;
	_skipNextLF = false;
}

bool ComUART::hasLine() const
{
	pollRx();
	return _lineReady;
}


size_t ComUART::readLine(char* outBuffer, size_t outBufferSize)
{
	if ((outBuffer == nullptr) || (outBufferSize == 0))
	{
		return 0;
	}

	pollRx();
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
	_skipNextLF = false;

	return copyLen;
}

bool ComUART::overflowed() const
{
	return _overflow;
}

void ComUART::clearLine()
{
	noInterrupts();
	_lineLength = 0;
	_lineBuffer[0] = '\0';
	_lineReady = false;
	_overflow = false;
	_skipNextLF = false;
	interrupts();
}

int ComUART::available()
{
	pollRx();
	if (_lineReady)
	{
		return static_cast<int>(_lineLength);
	}
	return _serialPort.available();
}

int ComUART::read()
{
	pollRx();
	if (_lineReady && (_lineLength > 0))
	{
		char c = _lineBuffer[0];
		for (size_t i = 1; i < _lineLength; ++i)
		{
			_lineBuffer[i - 1] = _lineBuffer[i];
		}
		--_lineLength;
		if (_lineLength == 0)
		{
			_lineReady = false;
		}
		return static_cast<uint8_t>(c);
	}

	return _serialPort.read();
}

void ComUART::pollRx() const
{
	while (_serialPort.available() > 0)
	{
		char c = static_cast<char>(_serialPort.read());

		if (_skipNextLF && (c == '\n'))
		{
			_skipNextLF = false;
			continue;
		}
		_skipNextLF = false;

		if (_lineReady)
		{
			continue;
		}

		if ((c == '\r') || (c == '\n'))
		{
			_lineReady = true;
			if (c == '\r')
			{
				_skipNextLF = true;
			}
			break;
		}

		if (_lineLength < (kLineBufferSize - 1))
		{
			_lineBuffer[_lineLength] = c;
			++_lineLength;
			_lineBuffer[_lineLength] = '\0';
		}
		else
		{
			_overflow = true;
		}
	}
}

size_t ComUART::write(uint8_t data)
{
	return _serialPort.write(data);
}

size_t ComUART::writeLine(const char* text)
{
	if (text == nullptr)
	{
		return 0;
	}

	const size_t written = _serialPort.print(text);
	_serialPort.print("\r\n");
	return written;
}

