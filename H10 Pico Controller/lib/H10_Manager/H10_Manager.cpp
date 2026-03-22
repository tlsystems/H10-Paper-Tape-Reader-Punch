#include "H10_Manager.h"

#include <SerialUART.h>
#include <hardware/irq.h>
#include <hardware/uart.h>

H10_Manager* H10_Manager::_activeInstance = nullptr;

H10_Manager::H10_Manager(SerialUART& serialPort, uint8_t rxPin,	uint8_t txPin,	uint8_t rtsPin,	uint8_t ctsPin)	:
	_serialPort(serialPort),
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

void H10_Manager::processCommand(UART_Communications& controller)
{
	if (!hasLine())
	{
		return;
	}

	parseCommand();

	char commandBuffer[kLineBufferSize];
	size_t commandLength = readLine(commandBuffer, sizeof(commandBuffer));

	if (commandLength > 0)
	{
		controller.processCommand(commandBuffer, commandLength);
	}
}

void H10_Manager::parseCommand()
{
	eCmd cmd = cmdInvalid;

	// transfer command to _command
	for (int i = 0; i < MAX_RX_LINE_LEN; i++)
	{
		if (_commandLine[i] == ' ')
		{
			_commandLine[i] = '\0';

			// set pointer to data
			_pData = &_commandLine[i + 1];
			break;
		}
		_command[i] = (char)_commandLine[i];
	}

	if		(strcmp(_command, "?") == 0)	cmd = cmdHelp;
	else if (strcmp(_command, "EC") == 0) 	cmd = cmdEcho;
	else if (strcmp(_command, "PB") == 0)	cmd = cmdPunchByte;
	else if (strcmp(_command, "PT") == 0)	cmd = cmdPunchText;
	else if (strcmp(_command, "PH") == 0)	cmd = cmdPunchHex;
	else if (strcmp(_command, "PL") == 0)	cmd = cmdPunchLeader;
	else if (strcmp(_command, "RB") == 0)	cmd = cmdReadByte;
	else if (strcmp(_command, "RH") == 0)	cmd = cmdReadHex;

	//diagPrintCommand();
	//SerialUSB.print(" was transferred\n");
}
void H10_Manager::doHelpCommand(CH10_Com* pCom)
{
	SerialUSB.write("doHelp\n");
	Serial1.write("doHelp\n");
	pCom->sendText("Commands:\n");
	return;
	pCom->sendText("  ?           Print Help\n");
	pCom->sendText("  PB <byte>   Punch byte\n");
	pCom->sendText("  PT <string> Punch text\n");
	pCom->sendText("  PH <record> Punch Intel Hex record\n");
	pCom->sendText("  PL <n>      Punch n chars of leader/trailer\n");
	pCom->sendText("  RB          Read byte\n");
	pCom->sendText("  RH <record> Read Intel Hex record\n");
	pCom->sendText("  ECHO <1|0>  Set Echo On/Off\n");
	_eState = sParseCommand;
	_retCode = rcNoError;
}

void H10_Manager::doEchoCommand(CH10_Com* pCom)
{
	pCom->setEcho(*_pData == '0');
	_eState = sParseCommand;
	_retCode = rcNoError;
}

void H10_Manager::doPunchByteCommand()
{
	//SerialUSB.println("doPunchByte");

	switch (_eState)
	{
	case sParseData:
		if (!parseByte(_pData, &_punchData))
		{
			
			_retCode =  rcBadData;
			return;
		}

		_eState = sPunchReadyWait;
		break;

	case sPunchReadyWait:
	case sPunchStartDwell:
		punchByte();
		break;

	case sPunchByteDone:
		if (_eState == sPunchByteDone)
			_eState = sParseCommand;
		break;

	case sNone:
	case sWaitForReaderReady:
		break;
	}

	//SerialUSB.print("  Byte: ");
	//SerialUSB.println(data);

	_retCode = rcNoError;
}

void H10_Manager::doPunchTextCommand()
{
	//SerialUSB.println("doPunchText");

	//int i = 0;
	//while (_pData[i] != '\0')
	//	punchCharacter(_pData[i]);

	//SerialUSB.print("  Text: ");
	//SerialUSB.println(_pData);

}

void H10_Manager::doPunchHexCommand()
{
	//parseIntelHex();
	//SerialUSB.println("doPunchHexCommand");
}

void H10_Manager::doPunchLeaderCommand()
{
	//SerialUSB.println("doPunchLeader");

	switch (_eState)
	{
	case sParseData:
		if (!parseByte(_pData, &_leaderCount))
		{
			_retCode = rcBadData;
			return;
		}

		_eState = sPunchReadyWait;
		break;

	case sPunchReadyWait:
	case sPunchStartDwell:
		_punchData = 0x00;
		punchByte();
		break;

	case sPunchByteDone:
		if (_eState == sPunchByteDone)
		{
			_leaderCount--;
			if (_leaderCount == 0)
				_eState = sParseCommand;
			else
				_eState = sPunchReadyWait;
		}
		break;

	case sNone:
	case sWaitForReaderReady:
		break;
	}

	//SerialUSB.print("  Leader: ");
	//for (int i = 0; i < count; i++)
	//{
	//	SerialUSB.write('.');
	//	; // call H10.printByte(0x00);
	//}
	//SerialUSB.write('\n');
}

void H10_Manager::doReadByteCommand()
{
	_retCode = rcInvalidCommand;
}

void H10_Manager::doReadHexCommand()
{
	_retCode = rcInvalidCommand;
}


