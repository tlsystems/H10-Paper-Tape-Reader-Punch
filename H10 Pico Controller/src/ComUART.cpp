#include "ComUART.h"

#include <SerialUART.h>
#include <hardware/irq.h>
#include <hardware/uart.h>

ComUART* ComUART::_activeInstance = nullptr;

ComUART::ComUART(SerialUART& serialPort, uint8_t rxPin,	uint8_t txPin,	uint8_t rtsPin,	uint8_t ctsPin)	:
	_serialPort(serialPort),
	_uart((&serialPort == &Serial1) ? uart0 : uart1),
	_rxPin(rxPin),
	_txPin(txPin),
	_rtsPin(rtsPin),
	_ctsPin(ctsPin),
	_lineLength(0),
	_lineReady(false),
	_overflow(false),
	_skipNextLF(false),
	_txHead(0),
	_txTail(0)
{
}

void ComUART::begin(uint32_t baudRate, uint16_t config)
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
	_txHead = 0;
	_txTail = 0;

	_activeInstance = this;
	irq_num_t irq = (_uart == uart0) ? UART0_IRQ : UART1_IRQ;
	irq_set_exclusive_handler(irq, ComUART::onUartIRQ);
	irq_set_enabled(irq, true);
	uart_set_irq_enables(_uart, true, false);
}

bool ComUART::hasLine() const
{
	return _lineReady;
}


size_t ComUART::readLine(char* outBuffer, size_t outBufferSize)
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
	uart_set_irq_enables(_uart, true, (_txHead != _txTail));
	interrupts();

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
	uart_set_irq_enables(_uart, true, (_txHead != _txTail));
	interrupts();
}

int ComUART::available()
{
	if (_lineReady)
	{
		return static_cast<int>(_lineLength);
	}
	return 0;
}

int ComUART::read()
{
	char c = 0;
	if (readLine(&c, 2) > 0)
	{
		return static_cast<uint8_t>(c);
	}
	return -1;
}

void ComUART::onUartIRQ()
{
	if (_activeInstance != nullptr)
	{	
		_activeInstance->haldleUartIRQ();
	}
}

void ComUART::haldleUartIRQ()
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
			uart_set_irq_enables(_uart, false, (_txHead != _txTail));
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

	while (uart_is_writable(_uart) && (_txTail != _txHead))
	{
		uart_putc_raw(_uart, static_cast<uint8_t>(_txBuffer[_txTail]));
		_txTail = (_txTail + 1U) % kTxBufferSize;
	}

	if (_txTail == _txHead)
	{
		uart_set_irq_enables(_uart, !_lineReady, false);
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

	size_t queuedChars = 0;
	noInterrupts();

	while (text[queuedChars] != '\0')
	{
		const size_t next = (_txHead + 1U) % kTxBufferSize;
		if (next == _txTail)
		{
			break;
		}

		_txBuffer[_txHead] = text[queuedChars];
		_txHead = next;
		++queuedChars;
	}

	for (size_t i = 0; i < 2; ++i)
	{
		const char terminator = (i == 0) ? '\r' : '\n';
		const size_t next = (_txHead + 1U) % kTxBufferSize;
		if (next == _txTail)
		{
			break;
		}

		_txBuffer[_txHead] = terminator;
		_txHead = next;
	}

	uart_set_irq_enables(_uart, !_lineReady, true);
	interrupts();

	return queuedChars;
}

