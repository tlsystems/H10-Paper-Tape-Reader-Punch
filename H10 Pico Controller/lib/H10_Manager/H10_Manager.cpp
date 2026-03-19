#include "H10_Manager.h"

#include <SerialUART.h>
#include <hardware/irq.h>
#include <hardware/uart.h>

H10_Manager* H10_Manager::_activeInstance = nullptr;

H10_Manager::H10_Manager(
	SerialUART& serialPort,
	uint8_t rxPin,
	uint8_t txPin,
	uint8_t rtsPin,
	uint8_t ctsPin)
	: _serialPort(serialPort),
	  _uart((&serialPort == &Serial1) ? uart0 : uart1),
	  _rxPin(rxPin),
	  _txPin(txPin),
	  _rtsPin(rtsPin),
	  _ctsPin(ctsPin),
	  _lineLength(0),
	  _lineReady(false),
	  _overflow(false),
	  _skipNextLF(false)
{
}

void H10_Manager::begin(uint32_t baudRate, uint16_t config)
{
	_serialPort.setRX(_rxPin);
	_serialPort.setTX(_txPin);
	_serialPort.setRTS(_rtsPin);
	_serialPort.setCTS(_ctsPin);
	_serialPort.begin(baudRate, config);

	_lineLength = 0;
	_lineReady = false;
	_overflow = false;
	_skipNextLF = false;

	_activeInstance = this;
	irq_num_t irq = (_uart == uart0) ? UART0_IRQ : UART1_IRQ;
	irq_set_exclusive_handler(irq, H10_Manager::onUartRxIRQ);
	irq_set_enabled(irq, true);
	uart_set_irq_enables(_uart, true, false);
}

bool H10_Manager::hasLine() const
{
	return _lineReady;
}

size_t H10_Manager::readLine(char* outBuffer, size_t outBufferSize)
{
	if ((outBuffer == nullptr) || (outBufferSize == 0))
	{
		return 0;
	}

	noInterrupts();
	if (!_lineReady)
	{
		interrupts();
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
	uart_set_irq_enables(_uart, true, false);
	interrupts();

	return copyLen;
}

bool H10_Manager::overflowed() const
{
	return _overflow;
}

void H10_Manager::clearLine()
{
	noInterrupts();
	_lineLength = 0;
	_lineReady = false;
	_overflow = false;
	_skipNextLF = false;
	uart_set_irq_enables(_uart, true, false);
	interrupts();
}

int H10_Manager::available()
{
	if (_lineReady)
	{
		return static_cast<int>(_lineLength);
	}
	return 0;
}

int H10_Manager::read()
{
	char c = 0;
	if (readLine(&c, 2) > 0)
	{
		return static_cast<uint8_t>(c);
	}
	return -1;
}

size_t H10_Manager::write(uint8_t data)
{
	return _serialPort.write(data);
}

void H10_Manager::onUartRxIRQ()
{
	if (_activeInstance != nullptr)
	{
		_activeInstance->handleUartRxIRQ();
	}
}

void H10_Manager::handleUartRxIRQ()
{
	while (uart_is_readable(_uart))
	{
		char c = static_cast<char>(uart_getc(_uart));

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
			uart_set_irq_enables(_uart, false, false);
			break;
		}

		if (_lineLength < kLineBufferSize)
		{
			_lineBuffer[_lineLength] = c;
			++_lineLength;
		}
		else
		{
			_overflow = true;
		}
	}
}

